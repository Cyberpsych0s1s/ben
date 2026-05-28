#ifndef EDITOR_STATE_H
#define EDITOR_STATE_H

#include "data_structures.h"


typedef enum {
    MODE_NORMAL,
    MODE_INSERT,
    MODE_COMMAND,
} EditorMode;

typedef struct {
    TextBuffer buffer;
    EditorMode current_mode;
    int top_line;                   // First visible line on screen
    int line_wrap_enabled;
    char temp_message[256];         // Temporary status messages
    const char *filename;           // (can be NULL)
} EditorState;

int  init_editor_state(EditorState *state, const char *filename);
void free_editor_state(EditorState *state);

void set_temp_message(EditorState *state, const char *message);
void clear_temp_message(EditorState *state);
int has_temp_message(const EditorState *state);

#endif
