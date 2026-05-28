// SPDX-License-Identifier: MIT
#ifdef _WIN32
#include <pdcurses.h>
#else
#include <ncurses.h>
#endif

#include "data_structures.h"
#include "editor_state.h"
#include "search.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

extern int get_absolute_line_number (const TextBuffer *buffer,
                                     Line *target_line);

void
init_search_state (SearchState *search_state)
{
  if (!search_state)
    return;

  search_state->search_term[0] = '\0';
  search_state->current_match_line = NULL;
  search_state->current_match_col = 0;
  search_state->has_active_search = 0;
  search_state->search_forward = 1;
  search_state->case_sensitive = 0;
}

char
to_lower (char c)
{
  return (c >= 'A' && c <= 'Z') ? c + 32 : c;
}

int
strncasecmp_custom (const char *s1, const char *s2, size_t n)
{
  for (size_t i = 0; i < n; i++)
    {
      char c1 = to_lower (s1[i]);
      char c2 = to_lower (s2[i]);

      if (c1 != c2)
        {
          return c1 - c2;
        }

      if (c1 == '\0')
        {
          break;
        }
    }
  return 0;
}

int
search_in_line (const Line *line, const char *term, size_t start_col,
                int case_sensitive, size_t *match_col)
{
  if (!line || !term || !match_col || strlen (term) == 0)
    {
      return 0;
    }

  char *line_text = line_to_string (line);
  if (!line_text)
    return 0;

  size_t line_len = strlen (line_text);
  size_t term_len = strlen (term);

  if (start_col >= line_len || term_len > line_len)
    {
      free (line_text);
      return 0;
    }

  for (size_t i = start_col; i <= line_len - term_len; i++)
    {
      int match;
      if (case_sensitive)
        {
          match = (strncmp (line_text + i, term, term_len) == 0);
        }
      else
        {
          match = (strncasecmp_custom (line_text + i, term, term_len) == 0);
        }

      if (match)
        {
          *match_col = i;
          free (line_text);
          return 1;
        }
    }

  free (line_text);
  return 0;
}

int
search_in_line_backward (const Line *line, const char *term, size_t start_col,
                         int case_sensitive, size_t *match_col)
{
  if (!line || !term || !match_col || strlen (term) == 0)
    {
      return 0;
    }

  char *line_text = line_to_string (line);
  if (!line_text)
    return 0;

  size_t line_len = strlen (line_text);
  size_t term_len = strlen (term);

  if (term_len > line_len)
    {
      free (line_text);
      return 0;
    }

  if (start_col >= line_len)
    {
      start_col = line_len - 1;
    }

  for (size_t i = start_col; i != SIZE_MAX; i--)
    {
      if (i + term_len > line_len)
        {
          continue;
        }

      int match;
      if (case_sensitive)
        {
          match = (strncmp (line_text + i, term, term_len) == 0);
        }
      else
        {
          match = (strncasecmp_custom (line_text + i, term, term_len) == 0);
        }

      if (match)
        {
          *match_col = i;
          free (line_text);
          return 1;
        }
    }

  free (line_text);
  return 0;
}

void
jump_to_match (EditorState *state, SearchState *search_state)
{
  if (!state || !search_state || !search_state->has_active_search
      || !search_state->current_match_line)
    {
      return;
    }

  state->buffer.current_line_node = search_state->current_match_line;
  state->buffer.current_col_offset = search_state->current_match_col;

  int max_row, max_col;
  getmaxyx (stdscr, max_row, max_col);
  int visible_lines = max_row - 2;

  int target_line_num = get_absolute_line_number (
      &state->buffer, search_state->current_match_line);

  if (target_line_num < state->top_line)
    {
      state->top_line = target_line_num - visible_lines / 4;
      if (state->top_line < 0)
        state->top_line = 0;
    }
  else if (target_line_num >= state->top_line + visible_lines)
    {
      state->top_line = target_line_num - (visible_lines * 3 / 4);
      if (state->top_line < 0)
        state->top_line = 0;
    }
}

int
perform_search (EditorState *state, SearchState *search_state,
                const char *term, int forward)
{
  if (!state || !search_state || !term || strlen (term) == 0)
    {
      return 0;
    }

  strncpy (search_state->search_term, term,
           sizeof (search_state->search_term) - 1);
  search_state->search_term[sizeof (search_state->search_term) - 1] = '\0';

  search_state->search_forward = forward;
  search_state->has_active_search = 1;

  Line *start_line = state->buffer.current_line_node;
  size_t start_col = state->buffer.current_col_offset;

  if (forward)
    {
      start_col++;

      size_t match_col;
      if (search_in_line (start_line, term, start_col,
                          search_state->case_sensitive, &match_col))
        {
          search_state->current_match_line = start_line;
          search_state->current_match_col = match_col;
          jump_to_match (state, search_state);
          return 1;
        }

      Line *current_line = start_line->next;
      while (current_line != NULL)
        {
          if (search_in_line (current_line, term, 0,
                              search_state->case_sensitive, &match_col))
            {
              search_state->current_match_line = current_line;
              search_state->current_match_col = match_col;
              jump_to_match (state, search_state);
              return 1;
            }
          current_line = current_line->next;
        }

      current_line = state->buffer.head;
      while (current_line != start_line && current_line != NULL)
        {
          if (search_in_line (current_line, term, 0,
                              search_state->case_sensitive, &match_col))
            {
              search_state->current_match_line = current_line;
              search_state->current_match_col = match_col;
              jump_to_match (state, search_state);
              return 1;
            }
          current_line = current_line->next;
        }

      if (search_in_line (start_line, term, 0, search_state->case_sensitive,
                          &match_col)
          && match_col < state->buffer.current_col_offset)
        {
          search_state->current_match_line = start_line;
          search_state->current_match_col = match_col;
          jump_to_match (state, search_state);
          return 1;
        }
    }
  else
    {
      if (start_col > 0)
        {
          start_col--;
        }

      size_t match_col;
      if (search_in_line_backward (start_line, term, start_col,
                                   search_state->case_sensitive, &match_col))
        {
          search_state->current_match_line = start_line;
          search_state->current_match_col = match_col;
          jump_to_match (state, search_state);
          return 1;
        }

      Line *current_line = start_line->prev;
      while (current_line != NULL)
        {
          size_t line_len = line_get_length (current_line);
          if (line_len > 0
              && search_in_line_backward (current_line, term, line_len - 1,
                                          search_state->case_sensitive,
                                          &match_col))
            {
              search_state->current_match_line = current_line;
              search_state->current_match_col = match_col;
              jump_to_match (state, search_state);
              return 1;
            }
          current_line = current_line->prev;
        }

      current_line = state->buffer.tail;
      while (current_line != start_line && current_line != NULL)
        {
          size_t line_len = line_get_length (current_line);
          if (line_len > 0
              && search_in_line_backward (current_line, term, line_len - 1,
                                          search_state->case_sensitive,
                                          &match_col))
            {
              search_state->current_match_line = current_line;
              search_state->current_match_col = match_col;
              jump_to_match (state, search_state);
              return 1;
            }
          current_line = current_line->prev;
        }

      size_t line_len = line_get_length (start_line);
      if (line_len > state->buffer.current_col_offset
          && search_in_line_backward (start_line, term, line_len - 1,
                                      search_state->case_sensitive, &match_col)
          && match_col > state->buffer.current_col_offset)
        {
          search_state->current_match_line = start_line;
          search_state->current_match_col = match_col;
          jump_to_match (state, search_state);
          return 1;
        }
    }

  return 0;
}

int
find_next_match (EditorState *state, SearchState *search_state)
{
  if (!state || !search_state || !search_state->has_active_search
      || strlen (search_state->search_term) == 0)
    {
      return 0;
    }

  if (!search_state->current_match_line)
    {
      return perform_search (state, search_state, search_state->search_term,
                             1);
    }

  Line *current_line = search_state->current_match_line;
  size_t start_col = search_state->current_match_col + 1;
  size_t match_col;

  if (search_in_line (current_line, search_state->search_term, start_col,
                      search_state->case_sensitive, &match_col))
    {
      search_state->current_match_col = match_col;
      jump_to_match (state, search_state);
      return 1;
    }

  current_line = current_line->next;
  while (current_line != NULL)
    {
      if (search_in_line (current_line, search_state->search_term, 0,
                          search_state->case_sensitive, &match_col))
        {
          search_state->current_match_line = current_line;
          search_state->current_match_col = match_col;
          jump_to_match (state, search_state);
          return 1;
        }
      current_line = current_line->next;
    }

  current_line = state->buffer.head;
  while (current_line != search_state->current_match_line
         && current_line != NULL)
    {
      if (search_in_line (current_line, search_state->search_term, 0,
                          search_state->case_sensitive, &match_col))
        {
          search_state->current_match_line = current_line;
          search_state->current_match_col = match_col;
          jump_to_match (state, search_state);
          return 1;
        }
      current_line = current_line->next;
    }

  if (search_in_line (search_state->current_match_line,
                      search_state->search_term, 0,
                      search_state->case_sensitive, &match_col)
      && match_col < search_state->current_match_col)
    {
      search_state->current_match_col = match_col;
      jump_to_match (state, search_state);
      return 1;
    }

  return 0;
}

int
find_previous_match (EditorState *state, SearchState *search_state)
{
  if (!state || !search_state || !search_state->has_active_search
      || strlen (search_state->search_term) == 0)
    {
      return 0;
    }

  if (!search_state->current_match_line)
    {
      return perform_search (state, search_state, search_state->search_term,
                             0);
    }

  Line *current_line = search_state->current_match_line;
  size_t start_col = (search_state->current_match_col > 0)
                         ? search_state->current_match_col - 1
                         : 0;
  size_t match_col;

  if (search_state->current_match_col > 0
      && search_in_line_backward (current_line, search_state->search_term,
                                  start_col, search_state->case_sensitive,
                                  &match_col))
    {
      search_state->current_match_col = match_col;
      jump_to_match (state, search_state);
      return 1;
    }

  current_line = current_line->prev;
  while (current_line != NULL)
    {
      size_t line_len = line_get_length (current_line);
      if (line_len > 0
          && search_in_line_backward (
              current_line, search_state->search_term, line_len - 1,
              search_state->case_sensitive, &match_col))
        {
          search_state->current_match_line = current_line;
          search_state->current_match_col = match_col;
          jump_to_match (state, search_state);
          return 1;
        }
      current_line = current_line->prev;
    }

  current_line = state->buffer.tail;
  while (current_line != search_state->current_match_line
         && current_line != NULL)
    {
      size_t line_len = line_get_length (current_line);
      if (line_len > 0
          && search_in_line_backward (
              current_line, search_state->search_term, line_len - 1,
              search_state->case_sensitive, &match_col))
        {
          search_state->current_match_line = current_line;
          search_state->current_match_col = match_col;
          jump_to_match (state, search_state);
          return 1;
        }
      current_line = current_line->prev;
    }

  size_t line_len = line_get_length (search_state->current_match_line);
  if (line_len > search_state->current_match_col
                     + strlen (search_state->search_term)
      && search_in_line_backward (search_state->current_match_line,
                                  search_state->search_term, line_len - 1,
                                  search_state->case_sensitive, &match_col)
      && match_col > search_state->current_match_col)
    {
      search_state->current_match_col = match_col;
      jump_to_match (state, search_state);
      return 1;
    }

  return 0;
}

void
clear_search (SearchState *search_state)
{
  if (!search_state)
    return;

  search_state->has_active_search = 0;
  search_state->current_match_line = NULL;
  search_state->current_match_col = 0;
}
