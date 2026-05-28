// SPDX-License-Identifier: MIT
#ifndef COLOR_CONFIG_H
#define COLOR_CONFIG_H

#ifdef _WIN32
#include <pdcurses.h>
#else
#include <ncurses.h>
#endif

typedef struct {
    short r,g,b; // 0-1000
} RGBColor;

typedef struct {
    RGBColor text_fg;
    RGBColor text_bg;
    RGBColor line_number_fg;
    RGBColor line_number_bg;
    RGBColor cursor_line_fg;
    RGBColor cursor_line_bg;
    RGBColor status_bar_fg;
    RGBColor status_bar_bg;
    RGBColor command_input_fg;
    RGBColor command_input_bg;

    RGBColor normal_mode_fg;
    RGBColor normal_mode_bg;
    RGBColor insert_mode_fg;
    RGBColor insert_mode_bg;
    RGBColor command_mode_fg;
    RGBColor command_mode_bg;
} ColorConfig;

/* Default palette derived from the Nord colour scheme (https://www.nordtheme.com).
 * Components are on ncurses' 0-1000 scale. */
static const ColorConfig DEFAULT_COLORS = {
    .text_fg           = {878, 906, 941},  // Nord4 - Snow Storm (light gray)
    .text_bg           = {188, 208, 243},  // Nord0 - Polar Night (dark blue-gray)
    .line_number_fg    = {376, 435, 522},  // Nord3 - Polar Night (medium gray)
    .line_number_bg    = {188, 208, 243},  // Nord0 - Polar Night (dark blue-gray)
    .cursor_line_fg    = {925, 937, 957},  // Nord6 - Snow Storm (white)
    .cursor_line_bg    = {231, 250, 273},  // Nord1 - Polar Night (slightly lighter)
    .status_bar_fg     = {188, 208, 243},  // Nord0 - Dark background
    .status_bar_bg     = {345, 424, 576},  // Nord10 - Frost (blue)
    .command_input_fg  = {878, 906, 941},  // Nord4 - Snow Storm (light gray)
    .command_input_bg  = {188, 208, 243},  // Nord0 - Polar Night (dark blue-gray)

    .normal_mode_fg    = {188, 208, 243},  // Nord0 - Dark text
    .normal_mode_bg    = {643, 745, 518},  // Nord14 - Aurora (green)
    .insert_mode_fg    = {925, 937, 957},  // Nord6 - Light text
    .insert_mode_bg    = {749, 380, 416},  // Nord11 - Aurora (red)
    .command_mode_fg   = {188, 208, 243},  // Nord0 - Dark text
    .command_mode_bg   = {922, 796, 545}   // Nord13 - Aurora (yellow)
};

// Don't change these numbers, they're used internally
#define COLOR_PAIR_TEXT         1
#define COLOR_PAIR_LINE_NUMBERS 2
#define COLOR_PAIR_STATUS_BAR   3
#define COLOR_PAIR_COMMAND      4
#define COLOR_PAIR_CURSOR_LINE  5
#define COLOR_PAIR_NORMAL_MODE  6
#define COLOR_PAIR_INSERT_MODE  7
#define COLOR_PAIR_COMMAND_MODE 8

void init_editor_colors(void);
void set_editor_colors(const ColorConfig *config);
const ColorConfig* get_current_colors(void);

#endif
