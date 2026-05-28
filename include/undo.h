#ifndef UNDO_H
#define UNDO_H

#include <stddef.h>
#include "data_structures.h"

#define MAX_UNDO_OPERATIONS 1000

typedef enum {
    UNDO_INSERT_CHAR,
    UNDO_DELETE_CHAR,
    UNDO_INSERT_LINE,
    UNDO_DELETE_LINE,
    UNDO_SPLIT_LINE,
    UNDO_MERGE_LINES
} UndoType;

typedef struct {
    UndoType type;
    Line *target_line;      // Direct pointer to the line (replaces line_num)
    size_t col_pos;
    char *data;             // Heap-allocated, owned by the op (may be NULL when data_len == 0)
    size_t data_len;
    int is_valid;          // Flag to mark if this operation is still valid
} UndoOperation;

/*
 * Ring buffer holding up to MAX_UNDO_OPERATIONS operations.
 *   tail        — index of the oldest operation in the ring
 *   count       — number of valid operations currently held [0..MAX]
 *   undo_depth  — number of operations currently undone (i.e. the redo stack
 *                 size); the newest op that can still be undone sits at offset
 *                 (count - undo_depth - 1) from tail.
 * Push overwrites the oldest entry when count == MAX.
 */
typedef struct {
    UndoOperation operations[MAX_UNDO_OPERATIONS];
    int tail;
    int count;
    int undo_depth;
} UndoStack;

extern UndoStack undo_stack;

void init_undo_system(void);
void push_undo_operation(UndoType type, Line *target_line, size_t col_pos, const char *data, size_t data_len);
int can_undo(void);
int can_redo(void);
void perform_undo(TextBuffer *buffer);
void perform_redo(TextBuffer *buffer);
void clear_redo_stack(void);
void invalidate_undo_operations_for_line(Line *deleted_line);
int is_line_valid_in_buffer(TextBuffer *buffer, Line *target_line);
size_t get_line_number(TextBuffer *buffer, Line *target);
Line* get_line_by_number(TextBuffer *buffer, size_t line_num);

#endif
