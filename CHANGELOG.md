# Changelog

All notable changes to ben are documented here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added
- Atomic file save — writes go through a `.tmp` file and `rename()`, so a
  crash or full disk leaves the original file intact.
- Vim-style save status messages: `"foo.txt" 42L, 1024B written`.
- "Out of memory" status-bar message instead of crashing on allocation
  failure.
- `atexit(restore_terminal)` so any exit path restores terminal state.
- `has_final_newline` tracking on `TextBuffer` — preserves the original
  file's trailing-newline state on save.
- `CONTRIBUTING.md` and this changelog.

### Fixed
- `saveToFile` no longer silently swallows `fopen` / `fprintf` / `fclose`
  failures. Errors are reported via `errno` and surfaced in the status bar.
- `:wq` now refuses to exit when the save fails. User must `:q!` to
  discard.
- Heap overflow in `gap_buffer_insert_char` on OOM — `ensure_capacity`
  failures are now propagated up instead of corrupting memory.
- `create_new_line` / `create_new_line_empty` no longer call `exit()` on
  OOM, which used to leave the terminal in raw mode and lose unsaved
  work.
- Undo ring buffer wraparound: index management was half-linear /
  half-circular and broke once the 1000-op ring filled. Rewritten with
  consistent modular arithmetic.
- Undo `data` payload is now heap-allocated, no longer truncated at 256
  bytes — splitting a long line and undoing restores the full content.
- Division by zero when the terminal is narrower than 8 columns.
- `gap_buffer_insert_string` is now a single `memcpy` instead of a
  per-character loop.
- `loadFromFile` no longer uses POSIX-only `getline` — replaced with a
  portable `fgets`-based reader, so the Windows build works.
- Test suite no longer depends on `<linux/limits.h>` (which doesn't exist
  on macOS or Windows).
- `:w  filename` (two or more spaces) no longer silently does nothing.

### Changed
- README rewritten with accurate clone URL, working Windows build
  instructions, and a roadmap.
- Makefile now works on bare Windows (PowerShell / `cmd.exe`) in addition
  to MSYS2 — `clean` and `install` are OS-aware.
- Undo `data` payload changed from inline `char[256]` to heap-allocated
  pointer. Cuts the ring-buffer array from ~280 KB to ~40 KB while
  removing the 256-byte truncation limit.
- Default colour palette comment cleaned up (was a self-deprecating note
  about ChatGPT).

### Removed
- `insert_line_after_buffer` — dead alias that just forwarded to
  `insert_line_after`. Call sites updated.
- `head` / `current` fields from `UndoStack` — replaced with cleaner
  `tail` / `count` / `undo_depth` ring-buffer state.
- Module-global `undo_stack` and `search_state` statics — both now live on
  `EditorState`, removing the single-buffer assumption.

### Changed
- `:command` dispatch refactored from a chained `else if (strcmp ...)` to
  a small `{name, takes_arg, handler}` table. Each handler returns
  `CMD_OK` / `CMD_QUIT` / `CMD_ERROR`. Adding a new `:command` is now a
  one-line change to the table.
- All undo functions now take an explicit `UndoStack *` parameter
  (`init_undo_system`, `push_undo_operation`, `can_undo`, `can_redo`,
  `perform_undo`, `perform_redo`, `clear_redo_stack`,
  `invalidate_undo_operations_for_line`). New `free_undo_system` for
  releasing heap-allocated op payloads.
- Render functions (`drawModeIndicator`, `drawTextContent`,
  `draw_line_with_search_highlight`) now take an explicit
  `const SearchState *` parameter.

### Added
- SPDX-License-Identifier MIT header on every source file (`include/`,
  `src/`, `tests/`).
