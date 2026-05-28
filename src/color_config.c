// SPDX-License-Identifier: MIT
#include "color_config.h"

// Global color configuration - can be modified at runtime
static ColorConfig current_colors;

void
init_editor_colors (void)
{
  // Initialize with default colors
  current_colors = DEFAULT_COLORS;

  // Check if terminal supports color customization
  if (can_change_color ())
    {
      // Apply custom colors
      init_color (10, current_colors.text_fg.r, current_colors.text_fg.g,
                  current_colors.text_fg.b);
      init_color (11, current_colors.text_bg.r, current_colors.text_bg.g,
                  current_colors.text_bg.b);
      init_color (12, current_colors.line_number_fg.r,
                  current_colors.line_number_fg.g,
                  current_colors.line_number_fg.b);
      init_color (13, current_colors.line_number_bg.r,
                  current_colors.line_number_bg.g,
                  current_colors.line_number_bg.b);
      init_color (14, current_colors.cursor_line_fg.r,
                  current_colors.cursor_line_fg.g,
                  current_colors.cursor_line_fg.b);
      init_color (15, current_colors.cursor_line_bg.r,
                  current_colors.cursor_line_bg.g,
                  current_colors.cursor_line_bg.b);
      init_color (16, current_colors.status_bar_fg.r,
                  current_colors.status_bar_fg.g,
                  current_colors.status_bar_fg.b);
      init_color (17, current_colors.status_bar_bg.r,
                  current_colors.status_bar_bg.g,
                  current_colors.status_bar_bg.b);
      init_color (18, current_colors.command_input_fg.r,
                  current_colors.command_input_fg.g,
                  current_colors.command_input_fg.b);
      init_color (19, current_colors.command_input_bg.r,
                  current_colors.command_input_bg.g,
                  current_colors.command_input_bg.b);
      init_color (20, current_colors.normal_mode_fg.r,
                  current_colors.normal_mode_fg.g,
                  current_colors.normal_mode_fg.b);
      init_color (21, current_colors.normal_mode_bg.r,
                  current_colors.normal_mode_bg.g,
                  current_colors.normal_mode_bg.b);
      init_color (22, current_colors.insert_mode_fg.r,
                  current_colors.insert_mode_fg.g,
                  current_colors.insert_mode_fg.b);
      init_color (23, current_colors.insert_mode_bg.r,
                  current_colors.insert_mode_bg.g,
                  current_colors.insert_mode_bg.b);
      init_color (24, current_colors.command_mode_fg.r,
                  current_colors.command_mode_fg.g,
                  current_colors.command_mode_fg.b);
      init_color (25, current_colors.command_mode_bg.r,
                  current_colors.command_mode_bg.g,
                  current_colors.command_mode_bg.b);

      // Initialize color pairs using our custom colors
      init_pair (COLOR_PAIR_TEXT, 10, 11);
      init_pair (COLOR_PAIR_LINE_NUMBERS, 12, 13);
      init_pair (COLOR_PAIR_CURSOR_LINE, 14, 15);
      init_pair (COLOR_PAIR_STATUS_BAR, 16, 17);
      init_pair (COLOR_PAIR_COMMAND, 18, 19);
      init_pair (COLOR_PAIR_NORMAL_MODE, 20, 21);
      init_pair (COLOR_PAIR_INSERT_MODE, 22, 23);
      init_pair (COLOR_PAIR_COMMAND_MODE, 24, 25);
    }
  else
    {
      // Fallback to basic ncurses colors if custom colors aren't supported
      init_pair (COLOR_PAIR_TEXT, COLOR_CYAN, COLOR_BLACK);
      init_pair (COLOR_PAIR_LINE_NUMBERS, COLOR_BLUE, COLOR_BLACK);
      init_pair (COLOR_PAIR_CURSOR_LINE, COLOR_WHITE, COLOR_BLACK);
      init_pair (COLOR_PAIR_STATUS_BAR, COLOR_WHITE, COLOR_BLUE);
      init_pair (COLOR_PAIR_COMMAND, COLOR_CYAN, COLOR_BLACK);
      init_pair (COLOR_PAIR_NORMAL_MODE, COLOR_BLACK, COLOR_GREEN);
      init_pair (COLOR_PAIR_INSERT_MODE, COLOR_WHITE, COLOR_RED);
      init_pair (COLOR_PAIR_COMMAND_MODE, COLOR_BLACK, COLOR_YELLOW);
    }
}

void
set_editor_colors (const ColorConfig *config)
{
  if (config)
    {
      current_colors = *config;
      init_editor_colors ();
    }
}

const ColorConfig *
get_current_colors (void)
{
  return &current_colors;
}
