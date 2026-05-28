// SPDX-License-Identifier: MIT

#include "data_structures.h"
#include "undo.h"
#include <stdlib.h>
#include <string.h>

/* Slot index (in operations[]) for the entry at the given offset from tail.
   offset is expected to be in [0, count). */
static inline int
ring_slot (const UndoStack *us, int offset_from_tail)
{
  return (us->tail + offset_from_tail) % MAX_UNDO_OPERATIONS;
}

/* Offset (from tail) of the newest undoable op, or -1 if there is none. */
static inline int
top_undo_offset (const UndoStack *us)
{
  return us->count - us->undo_depth - 1;
}

/* Offset (from tail) of the next op to redo, or -1 if there is none. */
static inline int
top_redo_offset (const UndoStack *us)
{
  return us->undo_depth > 0 ? us->count - us->undo_depth : -1;
}

/* Reset an op's payload, freeing any owned heap buffer. */
static void
release_op_data (UndoOperation *op)
{
  if (!op)
    return;
  free (op->data);
  op->data = NULL;
  op->data_len = 0;
  op->is_valid = 0;
  op->target_line = NULL;
}

void
init_undo_system (UndoStack *us)
{
  if (!us)
    return;
  /* Caller may pass a stale stack; explicitly NULL out the data pointers so
     release_op_data below doesn't free garbage. */
  for (int i = 0; i < MAX_UNDO_OPERATIONS; i++)
    {
      us->operations[i].data = NULL;
      us->operations[i].data_len = 0;
      us->operations[i].is_valid = 0;
      us->operations[i].target_line = NULL;
    }
  us->tail = 0;
  us->count = 0;
  us->undo_depth = 0;
}

void
free_undo_system (UndoStack *us)
{
  if (!us)
    return;
  for (int i = 0; i < MAX_UNDO_OPERATIONS; i++)
    {
      release_op_data (&us->operations[i]);
    }
  us->tail = 0;
  us->count = 0;
  us->undo_depth = 0;
}

void
push_undo_operation (UndoStack *us, UndoType type, Line *target_line,
                     size_t col_pos, const char *data, size_t data_len)
{
  if (!us)
    return;

  /* Pushing a fresh op invalidates any redo history. */
  clear_redo_stack (us);

  int slot;
  if (us->count < MAX_UNDO_OPERATIONS)
    {
      slot = ring_slot (us, us->count);
      us->count++;
    }
  else
    {
      /* Ring full — overwrite oldest, advance tail. */
      slot = us->tail;
      us->tail = (us->tail + 1) % MAX_UNDO_OPERATIONS;
    }

  UndoOperation *op = &us->operations[slot];
  release_op_data (op); /* drop any previous payload at this slot */

  op->type = type;
  op->target_line = target_line;
  op->col_pos = col_pos;
  op->data_len = 0;
  op->data = NULL;
  op->is_valid = 1;

  if (data && data_len > 0)
    {
      op->data = malloc (data_len + 1);
      if (!op->data)
        {
          /* OOM while recording history — drop the op rather than crash.
             User loses an undo step but state is intact. */
          op->is_valid = 0;
          return;
        }
      memcpy (op->data, data, data_len);
      op->data[data_len] = '\0';
      op->data_len = data_len;
    }
}

int
is_line_valid_in_buffer (TextBuffer *buffer, Line *target_line)
{
  if (!buffer || !target_line)
    return 0;

  Line *current = buffer->head;
  while (current != NULL)
    {
      if (current == target_line)
        {
          return 1;
        }
      current = current->next;
    }
  return 0;
}

void
invalidate_undo_operations_for_line (UndoStack *us, Line *deleted_line)
{
  if (!us || !deleted_line)
    return;

  /* Walk every physical slot — a stale reference can sit anywhere in the
     ring, not just inside the [tail, tail+count) active window. */
  for (int i = 0; i < MAX_UNDO_OPERATIONS; i++)
    {
      if (us->operations[i].target_line == deleted_line)
        {
          release_op_data (&us->operations[i]);
        }
    }
}

int
can_undo (const UndoStack *us)
{
  if (!us)
    return 0;
  int off = top_undo_offset (us);
  if (off < 0)
    return 0;
  return us->operations[ring_slot (us, off)].is_valid;
}

int
can_redo (const UndoStack *us)
{
  if (!us)
    return 0;
  int off = top_redo_offset (us);
  if (off < 0)
    return 0;
  return us->operations[ring_slot (us, off)].is_valid;
}

void
clear_redo_stack (UndoStack *us)
{
  if (!us)
    return;
  for (int i = 0; i < us->undo_depth; i++)
    {
      int off = us->count - 1 - i;
      us->operations[ring_slot (us, off)].is_valid = 0;
    }
  us->count -= us->undo_depth;
  us->undo_depth = 0;
}

void
validate_cursor_position (TextBuffer *buffer)
{
  if (!buffer)
    return;

  if (!buffer->head)
    {
      Line *initial_line = create_new_line_empty ();
      insert_line_at_end (buffer, initial_line);
      buffer->current_line_node = initial_line;
      buffer->current_col_offset = 0;
      return;
    }

  if (!buffer->current_line_node
      || !is_line_valid_in_buffer (buffer, buffer->current_line_node))
    {
      buffer->current_line_node = buffer->head;
      buffer->current_col_offset = 0;
      return;
    }

  if (buffer->current_line_node)
    {
      size_t line_len = line_get_length (buffer->current_line_node);
      if (buffer->current_col_offset > line_len)
        {
          buffer->current_col_offset = line_len;
        }
    }
}

void
perform_undo (UndoStack *us, TextBuffer *buffer)
{
  if (!can_undo (us) || !buffer)
    return;

  int off = top_undo_offset (us);
  UndoOperation *op = &us->operations[ring_slot (us, off)];

  if (op->type == UNDO_INSERT_LINE && op->target_line == NULL)
    {
      Line *to_remove = buffer->head;
      if (to_remove)
        {
          invalidate_undo_operations_for_line (us, to_remove);

          if (buffer->current_line_node == to_remove)
            {
              buffer->current_line_node = to_remove->next;
              buffer->current_col_offset = 0;
            }

          buffer->head = to_remove->next;
          if (buffer->head)
            {
              buffer->head->prev = NULL;
            }
          else
            {
              buffer->tail = NULL;
            }

          gap_buffer_destroy (to_remove->gb);
          free (to_remove);
          buffer->num_lines--;
        }
      us->undo_depth++;
      validate_cursor_position (buffer);
      return;
    }

  if (op->target_line
      && (!op->is_valid || !is_line_valid_in_buffer (buffer, op->target_line)))
    {
      op->is_valid = 0;
      us->undo_depth++; // Skip this invalid operation
      return;
    }

  Line *target_line = op->target_line;

  switch (op->type)
    {
    case UNDO_INSERT_CHAR:
      if (op->col_pos < line_get_length (target_line))
        {
          line_delete_char_at (target_line, op->col_pos);

          if (buffer->current_line_node == target_line
              && buffer->current_col_offset > op->col_pos)
            {
              buffer->current_col_offset--;
            }
        }
      break;

    case UNDO_DELETE_CHAR:
      if (op->col_pos <= line_get_length (target_line) && op->data_len > 0)
        {
          line_insert_char_at (target_line, op->col_pos, op->data[0]);

          if (buffer->current_line_node == target_line
              && buffer->current_col_offset > op->col_pos)
            {
              buffer->current_col_offset++;
            }
        }
      break;

    case UNDO_INSERT_LINE:
      {
        Line *to_remove = target_line->next;
        if (to_remove)
          {
            invalidate_undo_operations_for_line (us, to_remove);

            if (buffer->current_line_node == to_remove)
              {
                buffer->current_line_node = target_line;
                buffer->current_col_offset = line_get_length (target_line);
              }

            if (to_remove->next)
              {
                to_remove->next->prev = target_line;
              }
            target_line->next = to_remove->next;

            if (buffer->tail == to_remove)
              {
                buffer->tail = target_line;
              }

            gap_buffer_destroy (to_remove->gb);
            free (to_remove);
            buffer->num_lines--;
          }
        break;
      }

    case UNDO_DELETE_LINE:
      {
        Line *new_line = create_new_line (op->data);
        if (new_line)
          {
            insert_line_after (buffer, target_line, new_line);
          }
        break;
      }

    case UNDO_SPLIT_LINE:
      {
        Line *second_line = target_line->next;
        if (second_line)
          {
            char *second_content = line_to_string (second_line);
            if (second_content)
              {
                invalidate_undo_operations_for_line (us, second_line);

                if (buffer->current_line_node == second_line)
                  {
                    buffer->current_line_node = target_line;
                    buffer->current_col_offset
                        = op->col_pos + buffer->current_col_offset;
                  }

                line_insert_string_at (target_line, op->col_pos,
                                       second_content);
                free (second_content);

                target_line->next = second_line->next;
                if (second_line->next)
                  {
                    second_line->next->prev = target_line;
                  }
                else
                  {
                    buffer->tail = target_line;
                  }

                gap_buffer_destroy (second_line->gb);
                free (second_line);
                buffer->num_lines--;
              }
          }
        break;
      }

    case UNDO_MERGE_LINES:
      {
        size_t split_pos = op->col_pos;
        char *line_text = line_to_string (target_line);

        if (line_text && strlen (line_text) >= split_pos)
          {
            Line *new_line = create_new_line (line_text + split_pos);

            if (new_line)
              {
                gap_buffer_move_cursor_to (target_line->gb, split_pos);
                size_t chars_to_delete
                    = line_get_length (target_line) - split_pos;
                for (size_t i = 0; i < chars_to_delete; i++)
                  {
                    gap_buffer_delete_char (target_line->gb);
                  }

                insert_line_after (buffer, target_line, new_line);

                if (buffer->current_line_node == target_line
                    && buffer->current_col_offset > split_pos)
                  {
                    buffer->current_line_node = new_line;
                    buffer->current_col_offset -= split_pos;
                  }
              }
            free (line_text);
          }
        break;
      }
    }

  us->undo_depth++;
  validate_cursor_position (buffer);
}

void
perform_redo (UndoStack *us, TextBuffer *buffer)
{
  if (!can_redo (us) || !buffer)
    return;

  int off = top_redo_offset (us);
  UndoOperation *op = &us->operations[ring_slot (us, off)];

  if (!op->is_valid || !is_line_valid_in_buffer (buffer, op->target_line))
    {
      op->is_valid = 0;
      us->undo_depth--;
      return;
    }

  Line *target_line = op->target_line;

  switch (op->type)
    {
    case UNDO_INSERT_CHAR:
      if (op->col_pos <= line_get_length (target_line))
        {
          line_insert_char_at (target_line, op->col_pos, op->data[0]);

          if (buffer->current_line_node == target_line
              && buffer->current_col_offset > op->col_pos)
            {
              buffer->current_col_offset++;
            }
        }
      break;

    case UNDO_DELETE_CHAR:
      if (op->col_pos < line_get_length (target_line))
        {
          line_delete_char_at (target_line, op->col_pos);

          if (buffer->current_line_node == target_line
              && buffer->current_col_offset > op->col_pos)
            {
              buffer->current_col_offset--;
            }
        }
      break;

    case UNDO_INSERT_LINE:
      {
        Line *new_line = create_new_line (op->data);
        if (new_line)
          {
            insert_line_after (buffer, target_line, new_line);
          }
        break;
      }

    case UNDO_DELETE_LINE:
      {
        Line *to_remove = target_line->next;
        if (to_remove)
          {
            invalidate_undo_operations_for_line (us, to_remove);

            if (buffer->current_line_node == to_remove)
              {
                buffer->current_line_node = target_line;
                buffer->current_col_offset = line_get_length (target_line);
              }

            if (to_remove->next)
              {
                to_remove->next->prev = target_line;
              }
            target_line->next = to_remove->next;

            if (buffer->tail == to_remove)
              {
                buffer->tail = target_line;
              }

            gap_buffer_destroy (to_remove->gb);
            free (to_remove);
            buffer->num_lines--;
          }
        break;
      }

    case UNDO_SPLIT_LINE:
      {
        size_t split_pos = op->col_pos;
        char *line_text = line_to_string (target_line);

        if (line_text && strlen (line_text) >= split_pos)
          {
            Line *new_line = create_new_line (line_text + split_pos);

            if (new_line)
              {
                gap_buffer_move_cursor_to (target_line->gb, split_pos);
                size_t chars_to_delete
                    = line_get_length (target_line) - split_pos;
                for (size_t i = 0; i < chars_to_delete; i++)
                  {
                    gap_buffer_delete_char (target_line->gb);
                  }

                insert_line_after (buffer, target_line, new_line);

                if (buffer->current_line_node == target_line
                    && buffer->current_col_offset > split_pos)
                  {
                    buffer->current_line_node = new_line;
                    buffer->current_col_offset -= split_pos;
                  }
              }
            free (line_text);
          }
        break;
      }

    case UNDO_MERGE_LINES:
      {
        Line *second_line = target_line->next;
        if (second_line)
          {
            char *second_content = line_to_string (second_line);
            if (second_content)
              {
                invalidate_undo_operations_for_line (us, second_line);

                if (buffer->current_line_node == second_line)
                  {
                    buffer->current_line_node = target_line;
                    buffer->current_col_offset = line_get_length (target_line)
                                                 + buffer->current_col_offset;
                  }

                line_insert_string_at (target_line,
                                       line_get_length (target_line),
                                       second_content);
                free (second_content);

                target_line->next = second_line->next;
                if (second_line->next)
                  {
                    second_line->next->prev = target_line;
                  }
                else
                  {
                    buffer->tail = target_line;
                  }

                gap_buffer_destroy (second_line->gb);
                free (second_line);
                buffer->num_lines--;
              }
          }
        break;
      }
    }

  us->undo_depth--;
  validate_cursor_position (buffer);
}

/* Legacy helpers — kept for completeness but unused by the editor. */
size_t
get_line_number (TextBuffer *buffer, Line *target)
{
  if (!buffer || !target)
    return 0;

  Line *current = buffer->head;
  size_t line_num = 0;

  while (current != NULL && current != target)
    {
      current = current->next;
      line_num++;
    }

  if (current != target)
    return 0;
  return line_num;
}

Line *
get_line_by_number (TextBuffer *buffer, size_t line_num)
{
  if (!buffer || !buffer->head)
    return NULL;

  Line *current = buffer->head;
  size_t count = 0;

  while (current != NULL && count < line_num)
    {
      current = current->next;
      count++;
    }

  return current;
}
