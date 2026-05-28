
#ifdef _WIN32
#include <pdcurses.h>
#else
#include <ncurses.h>
#endif

#include "color_config.h"
#include "editor_state.h"
#include "search.h"
#include "text_editor_functions.h"
#include "undo.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static SearchState search_state;
static int search_initialized = 0;

int
get_absolute_line_number (const TextBuffer *buffer, Line *target_line)
{
  int line_num = 0;
  Line *current = buffer->head;

  while (current != NULL && current != target_line)
    {
      line_num++;
      current = current->next;
    }

  return line_num;
}

const char *
get_mode_string (EditorMode mode)
{
  switch (mode)
    {
    case MODE_NORMAL:
      return "NORMAL";
    case MODE_INSERT:
      return "INSERT";
    case MODE_COMMAND:
      return "COMMAND";
    default:
      return "UNKNOWN";
    }
}

void
drawModeIndicator (EditorMode mode, int line_wrap_enabled)
{
  int color_pair;
  const char *mode_text = get_mode_string (mode);

  switch (mode)
    {
    case MODE_NORMAL:
      color_pair = COLOR_PAIR_NORMAL_MODE;
      break;
    case MODE_INSERT:
      color_pair = COLOR_PAIR_INSERT_MODE;
      break;
    case MODE_COMMAND:
      color_pair = COLOR_PAIR_COMMAND_MODE;
      break;
    default:
      color_pair = COLOR_PAIR_NORMAL_MODE;
      break;
    }

  attron (COLOR_PAIR (color_pair));
  mvprintw (0, 0, " %s ", mode_text);
  attroff (COLOR_PAIR (color_pair));

  if (line_wrap_enabled)
    {
      mvprintw (0, strlen (mode_text) + 3, " [WRAP] ");
    }

  if (!search_initialized)
    {
      init_search_state (&search_state);
      search_initialized = 1;
    }

  if (search_state.has_active_search)
    {
      int search_pos = strlen (mode_text) + 3 + (line_wrap_enabled ? 8 : 0);
      mvprintw (0, search_pos, " [SEARCH: %s] ", search_state.search_term);
    }
}

int
get_wrapped_line_count (const char *text, int max_width, int line_wrap_enabled)
{
  if (!line_wrap_enabled || max_width <= 0)
    {
      return 1;
    }

  int len = strlen (text);
  if (len == 0)
    {
      return 1;
    }

  return (len + max_width - 1) / max_width;
}

void
draw_wrapped_line (int row, int col, const char *text, int max_width,
                   int color_pair, int line_wrap_enabled)
{
  if (max_width <= 0)
    return;

  if (!line_wrap_enabled)
    {
      attron (COLOR_PAIR (color_pair));
      mvprintw (row, col, "%.*s", max_width, text);
      attroff (COLOR_PAIR (color_pair));
      return;
    }

  int len = strlen (text);
  int current_row = row;
  int pos = 0;

  attron (COLOR_PAIR (color_pair));
  while (pos < len)
    {
      int chars_to_print = (len - pos > max_width) ? max_width : (len - pos);
      mvprintw (current_row, col, "%.*s", chars_to_print, text + pos);
      pos += chars_to_print;
      current_row++;
    }
  attroff (COLOR_PAIR (color_pair));
}

void
draw_line_with_search_highlight (int row, int col, const char *text,
                                 int max_width, int color_pair,
                                 int line_wrap_enabled, Line *line_node)
{
  if (!search_state.has_active_search || !text
      || strlen (search_state.search_term) == 0)
    {
      draw_wrapped_line (row, col, text, max_width, color_pair,
                         line_wrap_enabled);
      return;
    }

  int len = strlen (text);
  int term_len = strlen (search_state.search_term);
  int current_row = row;
  int pos = 0;

  while (pos < len
         && current_row < row
                              + get_wrapped_line_count (text, max_width,
                                                        line_wrap_enabled))
    {
      int line_end = pos + max_width;
      if (line_end > len)
        line_end = len;

      int segment_start = pos;
      int segment_end = line_end;

      attron (COLOR_PAIR (color_pair));

      for (int i = segment_start; i < segment_end; i++)
        {
          int is_match_start = 0;

          if (i + term_len <= len)
            {
              int match;
              if (search_state.case_sensitive)
                {
                  match
                      = (strncmp (text + i, search_state.search_term, term_len)
                         == 0);
                }
              else
                {
                  match = (strncasecmp_custom (
                               text + i, search_state.search_term, term_len)
                           == 0);
                }

              if (match)
                {
                  is_match_start = 1;
                  if (line_node == search_state.current_match_line
                      && i == (int)search_state.current_match_col)
                    {
                      attroff (COLOR_PAIR (color_pair));
                      attron (COLOR_PAIR (COLOR_PAIR_CURSOR_LINE));
                    }
                  else
                    {
                      attroff (COLOR_PAIR (color_pair));
                      attron (COLOR_PAIR (COLOR_PAIR_STATUS_BAR));
                    }
                }
            }

          mvaddch (current_row, col + (i - segment_start), text[i]);

          if (is_match_start)
            {
              for (int j = 1; j < term_len && i + j < segment_end; j++)
                {
                  mvaddch (current_row, col + (i + j - segment_start),
                           text[i + j]);
                }
              i += term_len - 1;

              attroff (COLOR_PAIR (COLOR_PAIR_CURSOR_LINE));
              attroff (COLOR_PAIR (COLOR_PAIR_STATUS_BAR));
              attron (COLOR_PAIR (color_pair));
            }
        }

      attroff (COLOR_PAIR (color_pair));

      if (line_wrap_enabled)
        {
          pos = segment_end;
          current_row++;
        }
      else
        {
          break;
        }
    }
}

void
drawLineNumbers (int visible_lines, const TextBuffer *buffer, int top_line)
{
  Line *current_line_node = buffer->head;
  int line_num = 1;
  int screen_row = 1; // Start from row 1 to leave space for mode indicator
  int max_col = getmaxx (stdscr);
  int text_width = max_col - 8; // Available width for text content

  for (int i = 0; i < top_line && current_line_node != NULL; ++i)
    {
      current_line_node = current_line_node->next;
      line_num++;
    }

  while (screen_row <= visible_lines && current_line_node != NULL)
    {
      char *line_text = line_to_string (current_line_node);
      int wrapped_lines
          = line_text ? get_wrapped_line_count (line_text, text_width, 1) : 1;
      if (line_text)
        free (line_text);

      attron (COLOR_PAIR (COLOR_PAIR_LINE_NUMBERS));
      mvprintw (screen_row, 1, "%4d", line_num);

      if (current_line_node == buffer->current_line_node)
        {
          attroff (COLOR_PAIR (COLOR_PAIR_LINE_NUMBERS));
          attron (COLOR_PAIR (COLOR_PAIR_CURSOR_LINE));
          mvprintw (screen_row, 5, "->");
          attroff (COLOR_PAIR (COLOR_PAIR_CURSOR_LINE));
        }
      else
        {
          mvprintw (screen_row, 5, "  ");
        }
      attroff (COLOR_PAIR (COLOR_PAIR_LINE_NUMBERS));

      for (int i = 1; i < wrapped_lines && screen_row + i <= visible_lines;
           i++)
        {
          mvprintw (screen_row + i, 1, "    ");
          mvprintw (screen_row + i, 5, "  ");
        }

      screen_row += wrapped_lines;
      current_line_node = current_line_node->next;
      line_num++;
    }
}

void
drawTextContent (int visible_lines, const TextBuffer *buffer, int top_line,
                 int line_wrap_enabled)
{
  Line *current_line_node = buffer->head;
  int screen_row = 1; // Start from row 1 to leave space for mode indicator
  int max_col = getmaxx (stdscr);
  int text_width = max_col - 8; // Available width for text content

  for (int i = 0; i < top_line && current_line_node != NULL; ++i)
    {
      current_line_node = current_line_node->next;
    }

  while (screen_row <= visible_lines && current_line_node != NULL)
    {
      char *line_text = line_to_string (current_line_node);
      if (line_text)
        {
          draw_line_with_search_highlight (
              screen_row, 8, line_text, text_width, COLOR_PAIR_TEXT,
              line_wrap_enabled, current_line_node);
          int wrapped_lines = get_wrapped_line_count (line_text, text_width,
                                                      line_wrap_enabled);
          screen_row += wrapped_lines;
          free (line_text);
        }
      else
        {
          screen_row++;
        }
      current_line_node = current_line_node->next;
    }
}

int
get_cursor_screen_row (const TextBuffer *buffer, int visible_lines,
                       int top_line, int line_wrap_enabled)
{
  Line *current_line_node = buffer->head;
  int line_count = 0;
  int screen_row = 1; // Start from row 1 to account for mode indicator
  int max_col = getmaxx (stdscr);
  int text_width = max_col - 8;

  for (int i = 0; i < top_line && current_line_node != NULL; ++i)
    {
      current_line_node = current_line_node->next;
      line_count++;
    }

  while (current_line_node != NULL
         && current_line_node != buffer->current_line_node)
    {
      char *line_text = line_to_string (current_line_node);
      int wrapped_lines = line_text
                              ? get_wrapped_line_count (line_text, text_width,
                                                        line_wrap_enabled)
                              : 1;
      if (line_text)
        free (line_text);

      screen_row += wrapped_lines;
      current_line_node = current_line_node->next;
      line_count++;
    }

  if (current_line_node == buffer->current_line_node && line_wrap_enabled
      && text_width > 0)
    {
      int cursor_wrapped_line = buffer->current_col_offset / text_width;
      screen_row += cursor_wrapped_line;
    }

  return screen_row;
}

void
drawStatusBar (const EditorState *state, const char *command)
{
  int max_row, max_col;
  getmaxyx (stdscr, max_row, max_col);
  int status_row = max_row - 1;

  attron (COLOR_PAIR (COLOR_PAIR_STATUS_BAR));

  mvhline (status_row, 0, ' ', max_col);

  if (state->filename != NULL && strlen (state->filename) > 0)
    {
      mvprintw (status_row, 1, "%s", state->filename);
    }
  else
    {
      mvprintw (status_row, 1, "[No Name]");
    }

  int cursor_line = get_absolute_line_number (&state->buffer,
                                              state->buffer.current_line_node)
                    + 1;
  int cursor_col = state->buffer.current_col_offset + 1;
  char position_text[50];
  snprintf (position_text, sizeof (position_text), "Line %d, Col %d",
            cursor_line, cursor_col);
  int pos_len = strlen (position_text);
  mvprintw (status_row, max_col - pos_len - 1, "%s", position_text);

  if (has_temp_message (state))
    {
      if (state->current_mode == MODE_COMMAND)
        {
        }
      else
        {
          attroff (COLOR_PAIR (COLOR_PAIR_STATUS_BAR));
          attron (COLOR_PAIR (COLOR_PAIR_COMMAND));

          int command_start = 20;
          int command_width = max_col - command_start - pos_len - 5;

          if (command_width > 0)
            {
              mvhline (status_row, command_start, ' ', command_width);
              mvprintw (status_row, command_start, "%s", state->temp_message);
            }

          attroff (COLOR_PAIR (COLOR_PAIR_COMMAND));
          attron (COLOR_PAIR (COLOR_PAIR_STATUS_BAR));
        }
    }

  if (state->current_mode == MODE_COMMAND && !has_temp_message (state)
      && command != NULL)
    {
      attroff (COLOR_PAIR (COLOR_PAIR_STATUS_BAR));
      attron (COLOR_PAIR (COLOR_PAIR_COMMAND));

      int command_start = 20;
      int command_width = max_col - command_start - pos_len - 5;

      if (command_width > 0)
        {
          mvhline (status_row, command_start, ' ', command_width);
          mvprintw (status_row, command_start, ":%s", command);

          int command_cursor_pos = command_start + 1 + strlen (command);
          if (command_cursor_pos < max_col - pos_len - 2)
            {
              mvaddch (status_row, command_cursor_pos, ' ' | A_BLINK);
            }
        }

      attroff (COLOR_PAIR (COLOR_PAIR_COMMAND));
      attron (COLOR_PAIR (COLOR_PAIR_STATUS_BAR));
    }

  attroff (COLOR_PAIR (COLOR_PAIR_STATUS_BAR));
}

void
handleInsertModeInput (int ch, EditorState *state)
{
  TextBuffer *buffer = &state->buffer;
  Line *line = buffer->current_line_node;
  size_t current_col = buffer->current_col_offset;
  int max_row, max_col;
  getmaxyx (stdscr, max_row, max_col);
  int visible_lines = max_row - 2;

  switch (ch)
    {
    case 27:
      state->current_mode = MODE_NORMAL;
      if (buffer->current_col_offset > 0)
        {
          buffer->current_col_offset--;
        }
      break;

    case 10:
      if (line != NULL)
        {
          char *line_text = line_to_string (line);
          if (line_text)
            {
              Line *new_line = create_new_line (line_text + current_col);
              if (!new_line)
                {
                  free (line_text);
                  set_temp_message (state, "Out of memory");
                  break;
                }

              push_undo_operation (UNDO_SPLIT_LINE, line, current_col,
                                   line_text + current_col,
                                   strlen (line_text + current_col));

              gap_buffer_move_cursor_to (line->gb, current_col);
              size_t chars_to_delete = line_get_length (line) - current_col;
              for (size_t i = 0; i < chars_to_delete; i++)
                {
                  gap_buffer_delete_char (line->gb);
                }

              insert_line_after (buffer, line, new_line);
              buffer->current_line_node = new_line;
              buffer->current_col_offset = 0;

              free (line_text);

              int cursor_screen_row = get_cursor_screen_row (
                  buffer, visible_lines, state->top_line,
                  state->line_wrap_enabled);
              if (cursor_screen_row > visible_lines)
                {
                  state->top_line++;
                }
            }
        }
      break;

    case KEY_BACKSPACE:
    case 127:
      if (current_col > 0)
        {
          char deleted_char = line_get_char_at (line, current_col - 1);
          push_undo_operation (UNDO_DELETE_CHAR, line, current_col - 1,
                               &deleted_char, 1);

          line_delete_char_before (line, current_col);
          buffer->current_col_offset--;
        }
      else if (line->prev != NULL)
        {
          Line *prev_line = line->prev;
          size_t prev_len = line_get_length (prev_line);
          char *current_text = line_to_string (line);

          if (current_text)
            {
              if (line_insert_string_at (prev_line, prev_len, current_text)
                  != 0)
                {
                  free (current_text);
                  set_temp_message (state, "Out of memory");
                  break;
                }
              push_undo_operation (UNDO_MERGE_LINES, prev_line, prev_len,
                                   current_text, strlen (current_text));
              free (current_text);
            }

          invalidate_undo_operations_for_line (line);

          prev_line->next = line->next;
          if (line->next != NULL)
            {
              line->next->prev = prev_line;
            }
          else
            {
              buffer->tail = prev_line;
            }
          gap_buffer_destroy (line->gb);
          free (line);
          buffer->num_lines--;

          buffer->current_line_node = prev_line;
          buffer->current_col_offset = prev_len;

          int cursor_screen_row
              = get_cursor_screen_row (buffer, visible_lines, state->top_line,
                                       state->line_wrap_enabled);
          if (cursor_screen_row < 1)
            {
              state->top_line--;
              if (state->top_line < 0)
                state->top_line = 0;
            }
        }
      break;

    case KEY_DC:
      if (current_col < line_get_length (line))
        {
          char deleted_char = line_get_char_at (line, current_col);
          push_undo_operation (UNDO_DELETE_CHAR, line, current_col,
                               &deleted_char, 1);

          line_delete_char_at (line, current_col);
        }
      else if (line->next != NULL)
        {
          Line *next_line = line->next;
          char *next_text = line_to_string (next_line);

          if (next_text)
            {
              size_t line_len_before = line_get_length (line);
              if (line_insert_string_at (line, line_len_before, next_text) != 0)
                {
                  free (next_text);
                  set_temp_message (state, "Out of memory");
                  break;
                }
              push_undo_operation (UNDO_MERGE_LINES, line, line_len_before,
                                   next_text, strlen (next_text));
              free (next_text);
            }

          invalidate_undo_operations_for_line (next_line);

          line->next = next_line->next;
          if (next_line->next != NULL)
            {
              next_line->next->prev = line;
            }
          else
            {
              buffer->tail = line;
            }
          gap_buffer_destroy (next_line->gb);
          free (next_line);
          buffer->num_lines--;
        }
      break;

    case KEY_UP:
      if (line->prev != NULL)
        {
          buffer->current_line_node = line->prev;
          size_t new_line_length = line_get_length (buffer->current_line_node);
          if (buffer->current_col_offset > new_line_length)
            {
              buffer->current_col_offset = new_line_length;
            }
          int cursor_screen_row
              = get_cursor_screen_row (buffer, visible_lines, state->top_line,
                                       state->line_wrap_enabled);
          if (cursor_screen_row < 1)
            {
              state->top_line--;
              if (state->top_line < 0)
                state->top_line = 0;
            }
        }
      break;

    case KEY_DOWN:
      if (line->next != NULL)
        {
          buffer->current_line_node = line->next;
          size_t new_line_length = line_get_length (buffer->current_line_node);
          if (buffer->current_col_offset > new_line_length)
            {
              buffer->current_col_offset = new_line_length;
            }
          int cursor_screen_row
              = get_cursor_screen_row (buffer, visible_lines, state->top_line,
                                       state->line_wrap_enabled);
          if (cursor_screen_row > visible_lines)
            {
              state->top_line++;
            }
        }
      break;

    case KEY_LEFT:
      if (buffer->current_col_offset > 0)
        {
          buffer->current_col_offset--;
        }
      break;

    case KEY_RIGHT:
      if (buffer->current_col_offset < line_get_length (line))
        {
          buffer->current_col_offset++;
        }
      break;

    default:
      if (isprint (ch))
        {
          char inserted = (char)ch;
          if (line_insert_char_at (line, current_col, inserted) != 0)
            {
              set_temp_message (state, "Out of memory");
              break;
            }
          push_undo_operation (UNDO_INSERT_CHAR, line, current_col, &inserted,
                               1);
          buffer->current_col_offset++;
        }
      break;
    }
}

void
handleNormalModeInput (int ch, EditorState *state)
{
  TextBuffer *buffer = &state->buffer;
  Line *line = buffer->current_line_node;
  size_t current_col = buffer->current_col_offset;
  int max_row, max_col;
  getmaxyx (stdscr, max_row, max_col);
  int visible_lines = max_row - 2;

  if (!search_initialized)
    {
      init_search_state (&search_state);
      search_initialized = 1;
    }

  switch (ch)
    {
    case 'h':
      if (buffer->current_col_offset > 0)
        {
          buffer->current_col_offset--;
        }
      break;
    case 'j':
      if (line->next != NULL)
        {
          buffer->current_line_node = line->next;
          size_t new_line_length = line_get_length (buffer->current_line_node);
          if (buffer->current_col_offset > new_line_length)
            {
              buffer->current_col_offset = new_line_length;
            }
          int cursor_screen_row
              = get_cursor_screen_row (buffer, visible_lines, state->top_line,
                                       state->line_wrap_enabled);
          if (cursor_screen_row > visible_lines)
            {
              state->top_line++;
            }
        }
      break;
    case 'k':
      if (line->prev != NULL)
        {
          buffer->current_line_node = line->prev;
          size_t new_line_length = line_get_length (buffer->current_line_node);
          if (buffer->current_col_offset > new_line_length)
            {
              buffer->current_col_offset = new_line_length;
            }
          if (get_absolute_line_number (buffer, buffer->current_line_node)
              < state->top_line)
            {
              state->top_line--;
            }
        }
      break;
    case 'l':
      if (buffer->current_col_offset < line_get_length (line))
        {
          buffer->current_col_offset++;
        }
      break;

    case KEY_UP:
      handleNormalModeInput ('k', state);
      break;
    case KEY_DOWN:
      handleNormalModeInput ('j', state);
      break;
    case KEY_LEFT:
      handleNormalModeInput ('h', state);
      break;
    case KEY_RIGHT:
      handleNormalModeInput ('l', state);
      break;

    case '/':
      state->current_mode = MODE_COMMAND;
      clear_temp_message (state);
      break;

    case '?':
      state->current_mode = MODE_COMMAND;
      clear_temp_message (state);
      break;

    case 'n':
      if (search_state.has_active_search)
        {
          if (find_next_match (state, &search_state))
            {
              set_temp_message (state, "Found next match");
            }
          else
            {
              set_temp_message (state, "Search wrapped to beginning");
            }
        }
      else
        {
          set_temp_message (state, "No active search");
        }
      break;

    case 'N':
      if (search_state.has_active_search)
        {
          if (find_previous_match (state, &search_state))
            {
              set_temp_message (state, "Found previous match");
            }
          else
            {
              set_temp_message (state, "Search wrapped to end");
            }
        }
      else
        {
          set_temp_message (state, "No active search");
        }
      break;

    case 'i':
      state->current_mode = MODE_INSERT;
      break;
    case 'a':
      if (buffer->current_col_offset < line_get_length (line))
        {
          buffer->current_col_offset++;
        }
      state->current_mode = MODE_INSERT;
      break;
    case 'A':
      buffer->current_col_offset = line_get_length (line);
      state->current_mode = MODE_INSERT;
      break;

    case 'o':
      {
        Line *new_line = create_new_line_empty ();
        if (!new_line)
          {
            set_temp_message (state, "Out of memory");
            break;
          }
        push_undo_operation (UNDO_INSERT_LINE, line, 0, "", 0);

        insert_line_after (buffer, line, new_line);
        buffer->current_line_node = new_line;
        buffer->current_col_offset = 0;
        state->current_mode = MODE_INSERT;

        int cursor_screen_row = get_cursor_screen_row (
            buffer, visible_lines, state->top_line, state->line_wrap_enabled);
        if (cursor_screen_row > visible_lines)
          {
            state->top_line++;
          }
      }
      break;

    case 'O':
      {
        Line *new_line = create_new_line_empty ();
        if (!new_line)
          {
            set_temp_message (state, "Out of memory");
            break;
          }

        if (line->prev != NULL)
          {
            push_undo_operation (UNDO_INSERT_LINE, line->prev, 0, "", 0);
            insert_line_after (buffer, line->prev, new_line);
          }
        else
          {
            push_undo_operation (UNDO_INSERT_LINE, NULL, 0, "", 0);
            new_line->next = buffer->head;
            if (buffer->head != NULL)
              {
                buffer->head->prev = new_line;
              }
            buffer->head = new_line;
            if (buffer->tail == NULL)
              {
                buffer->tail = new_line;
              }
            buffer->num_lines++;
          }

        buffer->current_line_node = new_line;
        buffer->current_col_offset = 0;
        state->current_mode = MODE_INSERT;

        int cursor_screen_row = get_cursor_screen_row (
            buffer, visible_lines, state->top_line, state->line_wrap_enabled);
        if (cursor_screen_row < 1)
          {
            state->top_line--;
            if (state->top_line < 0)
              state->top_line = 0;
          }
      }
      break;

    case ':':
      state->current_mode = MODE_COMMAND;
      clear_temp_message (state);
      break;

    case 27:
      state->current_mode = MODE_NORMAL;
      clear_temp_message (state);
      break;

    case 'w':
      state->line_wrap_enabled = !state->line_wrap_enabled;
      set_temp_message (state, state->line_wrap_enabled
                                   ? "Line wrap enabled"
                                   : "Line wrap disabled");
      break;

    case 'x':
      if (current_col < line_get_length (line))
        {
          char deleted_char = line_get_char_at (line, current_col);
          push_undo_operation (UNDO_DELETE_CHAR, line, current_col,
                               &deleted_char, 1);
          line_delete_char_at (line, current_col);
        }
      break;

    case 'X':
      if (current_col > 0)
        {
          char deleted_char = line_get_char_at (line, current_col - 1);
          push_undo_operation (UNDO_DELETE_CHAR, line, current_col - 1,
                               &deleted_char, 1);
          line_delete_char_before (line, current_col);
          buffer->current_col_offset--;
        }
      break;

    case 'u':
      if (can_undo ())
        {
          perform_undo (buffer);
          set_temp_message (state, "Undo successful");
        }
      else
        {
          set_temp_message (state, "Nothing to undo");
        }
      break;

    case 18:
      if (can_redo ())
        {
          perform_redo (buffer);
          set_temp_message (state, "Redo successful");
        }
      else
        {
          set_temp_message (state, "Nothing to redo");
        }
      break;
    }
}

void
handleCommandModeInput (int ch, char *command, EditorState *state)
{
  static int command_index = 0;
  static int is_search_command = 0; // Track if this is a search command
  static int search_direction = 1;  // 1 for forward, 0 for backward
  TextBuffer *buffer = &state->buffer;

  if (!search_initialized)
    {
      init_search_state (&search_state);
      search_initialized = 1;
    }

  if (command_index == 0 && (ch == '/' || ch == '?'))
    {
      is_search_command = 1;
      search_direction = (ch == '/') ? 1 : 0;
      command[command_index++] = ch;
      command[command_index] = '\0';
      return;
    }

  switch (ch)
    {
    case 10:
      if (is_search_command && command_index > 1)
        {
          char *search_term = command + 1;

          if (strlen (search_term) > 0)
            {
              if (perform_search (state, &search_state, search_term,
                                  search_direction))
                {
                  char msg[100];
                  snprintf (msg, sizeof (msg), "Found: %s", search_term);
                  set_temp_message (state, msg);
                }
              else
                {
                  set_temp_message (state, "Pattern not found");
                }
            }
          else
            {
              if (search_state.has_active_search
                  && strlen (search_state.search_term) > 0)
                {
                  if (search_direction)
                    {
                      find_next_match (state, &search_state);
                    }
                  else
                    {
                      find_previous_match (state, &search_state);
                    }
                  set_temp_message (state, "Repeated last search");
                }
              else
                {
                  set_temp_message (state, "No previous search");
                }
            }
        }
      else if (strcmp (command, "q") == 0)
        {
          endwin ();
          exit (EXIT_SUCCESS);
        }
      else if (strcmp (command, "w") == 0)
        {
          if (state->filename != NULL && strlen (state->filename) > 0)
            {
              int lines = 0;
              size_t bytes = 0;
              char msg[256];
              if (saveToFile (state->filename, buffer, &lines, &bytes) == 0)
                {
                  snprintf (msg, sizeof (msg), "\"%s\" %dL, %luB written",
                            state->filename, lines,
                            (unsigned long)bytes);
                }
              else
                {
                  snprintf (msg, sizeof (msg), "Error writing \"%s\": %s",
                            state->filename, strerror (errno));
                }
              set_temp_message (state, msg);
            }
          else
            {
              set_temp_message (state, "Error: No filename specified");
            }
        }
      else if (strncmp (command, "w ", 2) == 0)
        {
          const char *save_filename = command + 2; // Skip "w "
          while (*save_filename == ' ')
            save_filename++;
          if (strlen (save_filename) > 0)
            {
              int lines = 0;
              size_t bytes = 0;
              char msg[256];
              if (saveToFile (save_filename, buffer, &lines, &bytes) == 0)
                {
                  snprintf (msg, sizeof (msg), "\"%s\" %dL, %luB written",
                            save_filename, lines, (unsigned long)bytes);
                }
              else
                {
                  snprintf (msg, sizeof (msg), "Error writing \"%s\": %s",
                            save_filename, strerror (errno));
                }
              set_temp_message (state, msg);
            }
          else
            {
              set_temp_message (state, "Error: No filename specified");
            }
        }
      else if (strcmp (command, "wq") == 0)
        {
          if (state->filename == NULL || strlen (state->filename) == 0)
            {
              set_temp_message (state,
                                "Error: No filename (use :wq <name> or :q!)");
            }
          else
            {
              int lines = 0;
              size_t bytes = 0;
              if (saveToFile (state->filename, buffer, &lines, &bytes) == 0)
                {
                  endwin ();
                  exit (EXIT_SUCCESS);
                }
              char msg[256];
              snprintf (msg, sizeof (msg), "Error writing \"%s\": %s",
                        state->filename, strerror (errno));
              set_temp_message (state, msg);
            }
        }
      else if (strncmp (command, "wq ", 3) == 0)
        {
          const char *save_filename = command + 3; // Skip "wq "
          while (*save_filename == ' ')
            save_filename++;
          if (strlen (save_filename) == 0)
            {
              set_temp_message (state, "Error: No filename specified");
            }
          else
            {
              int lines = 0;
              size_t bytes = 0;
              if (saveToFile (save_filename, buffer, &lines, &bytes) == 0)
                {
                  endwin ();
                  exit (EXIT_SUCCESS);
                }
              char msg[256];
              snprintf (msg, sizeof (msg), "Error writing \"%s\": %s",
                        save_filename, strerror (errno));
              set_temp_message (state, msg);
            }
        }
      else if (strcmp (command, "wrap") == 0)
        {
          state->line_wrap_enabled = 1;
          set_temp_message (state, "Line wrap enabled");
        }
      else if (strcmp (command, "nowrap") == 0)
        {
          state->line_wrap_enabled = 0;
          set_temp_message (state, "Line wrap disabled");
        }
      else if (strcmp (command, "nohl") == 0
               || strcmp (command, "nohlsearch") == 0)
        {
          clear_search (&search_state);
          set_temp_message (state, "Search cleared");
        }
      else if (strcmp (command, "set ic") == 0)
        {
          search_state.case_sensitive = 0;
          set_temp_message (state, "Search is now case insensitive");
        }
      else if (strcmp (command, "set noic") == 0)
        {
          search_state.case_sensitive = 1;
          set_temp_message (state, "Search is now case sensitive");
        }
      else if (!is_search_command)
        {
          set_temp_message (state, "Unknown command");
        }

      command[0] = '\0'; // Reset command buffer
      command_index = 0; // Reset command index
      is_search_command = 0;
      state->current_mode = MODE_NORMAL;
      break;

    case 27:
      command[0] = '\0';
      command_index = 0;
      is_search_command = 0;
      state->current_mode = MODE_NORMAL;
      break;

    case KEY_BACKSPACE:
    case 127:
      if (command_index > 0)
        {
          command_index--;
          command[command_index] = '\0';

          if (is_search_command && command_index == 0)
            {
              is_search_command = 0;
            }
        }
      break;

    default:
      if (isprint (ch) && command_index < MAX_COMMAND_LENGTH - 1)
        {
          command[command_index++] = ch;
          command[command_index] = '\0';
        }
      break;
    }
}

void
handleInput (char *command, EditorState *state)
{
  int ch = getch ();

  switch (state->current_mode)
    {
    case MODE_NORMAL:
      handleNormalModeInput (ch, state);
      break;
    case MODE_INSERT:
      handleInsertModeInput (ch, state);
      break;
    case MODE_COMMAND:
      handleCommandModeInput (ch, command, state);
      break;
    }
}
