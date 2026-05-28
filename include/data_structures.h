// SPDX-License-Identifier: MIT
#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <stddef.h>
#include "gap_buffer.h"

typedef struct Line {
    GapBuffer *gb;
    struct Line *next;
    struct Line *prev;
} Line;

typedef struct TextBuffer {
    Line *head;
    Line *tail;
    size_t num_lines;
    Line *current_line_node;
    size_t current_col_offset;
    int has_final_newline;        // 1 if file ends with '\n' (POSIX default), 0 if not
} TextBuffer;

void init_editor_buffer(TextBuffer *buffer);
Line* create_new_line(const char *content);
Line* create_new_line_empty();
void insert_line_after(TextBuffer *buffer, Line *prev_line, Line *new_line);
void insert_line_at_beginning(TextBuffer *buffer, Line *new_line);
void free_editor_buffer(TextBuffer *buffer);
void insert_line_at_end(TextBuffer *buffer, Line *new_line);

size_t line_get_length(const Line *line);
char line_get_char_at(const Line *line, size_t position);
char* line_to_string(const Line *line);
int  line_insert_char_at(Line *line, size_t position, char c);
int  line_insert_string_at(Line *line, size_t position, const char *str);
void line_delete_char_at(Line *line, size_t position);
void line_delete_char_before(Line *line, size_t position);

#endif
