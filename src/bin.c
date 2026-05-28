// SPDX-License-Identifier: MIT
#ifdef _WIN32
#include <pdcurses.h>
#else
#include <curses.h>
#endif

#include "color_config.h"
#include "data_structures.h"
#include "editor_state.h"
#include "text_editor_functions.h"
#include "undo.h"
#include <stdio.h>
#include <stdlib.h>

static void
restore_terminal (void)
{
  if (!isendwin ())
    endwin ();
}

int
main (int argc, char *argv[])
{
  initscr ();
  atexit (restore_terminal);

#ifndef _WIN32
  set_escdelay (25);
#endif

  start_color ();
  init_editor_colors ();
  /* Make every unpainted cell (including post-clear() background fill)
     adopt the editor's text colour pair, so the theme covers the whole
     terminal instead of bleeding through to the host's defaults. */
  bkgd (' ' | COLOR_PAIR (COLOR_PAIR_TEXT));
  cbreak ();
  keypad (stdscr, TRUE);
  noecho ();

  EditorState editor_state;
  const char *filename = (argc > 1) ? argv[1] : NULL;
  if (init_editor_state (&editor_state, filename) != 0)
    {
      endwin ();
      fprintf (stderr, "ben: out of memory while initializing editor\n");
      return EXIT_FAILURE;
    }

  char command[MAX_COMMAND_LENGTH] = "";

  while (1)
    {
      int max_row = getmaxy (stdscr);
      int max_col = getmaxx (stdscr);
      int visible_lines = max_row - 2;
      int text_width = max_col - 8;
      clear ();

      drawModeIndicator (editor_state.current_mode,
                         editor_state.line_wrap_enabled, &editor_state.search);

      attron (COLOR_PAIR (COLOR_PAIR_LINE_NUMBERS));
      drawLineNumbers (visible_lines, &editor_state.buffer,
                       editor_state.top_line);
      attroff (COLOR_PAIR (COLOR_PAIR_LINE_NUMBERS));
      drawTextContent (visible_lines, &editor_state.buffer,
                       editor_state.top_line, editor_state.line_wrap_enabled,
                       &editor_state.search);

      drawStatusBar (&editor_state, editor_state.current_mode == MODE_COMMAND
                                        ? command
                                        : NULL);

      int cursor_screen_row = get_cursor_screen_row (
          &editor_state.buffer, visible_lines, editor_state.top_line,
          editor_state.line_wrap_enabled);
      int cursor_screen_col;

      if (editor_state.line_wrap_enabled
          && editor_state.buffer.current_line_node != NULL && text_width > 0)
        {
          cursor_screen_col
              = 8 + (editor_state.buffer.current_col_offset % text_width);
        }
      else
        {
          cursor_screen_col = editor_state.buffer.current_col_offset + 8;
        }

      if (cursor_screen_row < 1)
        {
          editor_state.top_line -= (1 - cursor_screen_row);
          if (editor_state.top_line < 0)
            editor_state.top_line = 0;
          cursor_screen_row = 1;
        }
      else if (cursor_screen_row > visible_lines)
        {
          editor_state.top_line += (cursor_screen_row - visible_lines);
          cursor_screen_row = visible_lines;
        }

      move (cursor_screen_row, cursor_screen_col);
      refresh ();

      handleInput (command, &editor_state);
    }

  endwin ();
  free_editor_state (&editor_state);
  return 0;
}
