// SPDX-License-Identifier: MIT
#include "editor_state.h"
#include "text_editor_functions.h"
#include <string.h>

int
init_editor_state (EditorState *state, const char *filename)
{
  if (!state)
    return -1;

  init_editor_buffer (&state->buffer);
  init_undo_system (&state->undo);
  init_search_state (&state->search);

  state->current_mode = MODE_NORMAL;
  state->top_line = 0;
  state->line_wrap_enabled = 1;
  state->temp_message[0] = '\0';
  state->filename = filename;

  if (filename)
    {
      loadFromFile (filename, &state->buffer);
    }
  else
    {
      Line *initial_line = create_new_line ("");
      if (!initial_line)
        return -1;
      insert_line_at_end (&state->buffer, initial_line);
      state->buffer.current_line_node = initial_line;
    }

  if (state->buffer.head == NULL)
    {
      Line *initial_line = create_new_line ("");
      if (!initial_line)
        return -1;
      insert_line_at_end (&state->buffer, initial_line);
      state->buffer.current_line_node = initial_line;
    }
  if (state->buffer.current_line_node == NULL)
    {
      state->buffer.current_line_node = state->buffer.head;
      state->buffer.current_col_offset = 0;
    }
  return 0;
}

void
free_editor_state (EditorState *state)
{
  if (!state)
    return;

  free_undo_system (&state->undo);
  free_editor_buffer (&state->buffer);
}

void
set_temp_message (EditorState *state, const char *message)
{
  if (!state || !message)
    return;

  strncpy (state->temp_message, message, sizeof (state->temp_message) - 1);
  state->temp_message[sizeof (state->temp_message) - 1] = '\0';
}

void
clear_temp_message (EditorState *state)
{
  if (!state)
    return;

  state->temp_message[0] = '\0';
}

int
has_temp_message (const EditorState *state)
{
  if (!state)
    return 0;

  return state->temp_message[0] != '\0';
}
