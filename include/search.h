// SPDX-License-Identifier: MIT
#ifndef SEARCH_H
#define SEARCH_H

#include "data_structures.h"

struct EditorStateTag;        /* forward declaration; full type in editor_state.h */
typedef struct EditorStateTag EditorState;

#define MAX_SEARCH_TERM_LENGTH 256

typedef struct {
    char search_term[MAX_SEARCH_TERM_LENGTH];
    Line *current_match_line;
    size_t current_match_col;
    int has_active_search;
    int search_forward;  // 1 for forward, 0 for backward
    int case_sensitive;  // 1 for case sensitive, 0 for case insensitive
} SearchState;

void init_search_state(SearchState *search_state);
int perform_search(EditorState *state, SearchState *search_state, const char *term, int forward);
int find_next_match(EditorState *state, SearchState *search_state);
int find_previous_match(EditorState *state, SearchState *search_state);
void clear_search(SearchState *search_state);

int search_in_line(const Line *line, const char *term, size_t start_col, int case_sensitive, size_t *match_col);
int search_in_line_backward(const Line *line, const char *term, size_t start_col, int case_sensitive, size_t *match_col);
void jump_to_match(EditorState *state, SearchState *search_state);

char to_lower(char c);
int strncasecmp_custom(const char *s1, const char *s2, size_t n);

#endif
