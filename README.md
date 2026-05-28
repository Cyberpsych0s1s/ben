# ben

A minimalist, Vim-inspired terminal text editor written in C.

Modal editing, gap-buffer–backed lines, multi-level undo/redo, atomic file
writes, and incremental search — all in ~3,300 lines of C99 with ncurses
(or PDCurses on Windows).

## Features

- Modal editing — `NORMAL`, `INSERT`, `COMMAND` modes with familiar Vim
  keystrokes.
- Gap-buffer per line, doubly-linked line list — efficient inserts/deletes.
- Multi-level undo and redo (up to 1000 operations) with line-aware
  invalidation.
- Forward and backward incremental search (`/`, `?`, `n`, `N`) with
  case-insensitive option.
- Soft line wrap, toggleable per session or via `:wrap` / `:nowrap`.
- Atomic save — writes go through a `.tmp` file and `rename()`, so a crash
  or full disk leaves your original file intact.
- Cross-platform — Linux, macOS, and Windows (via MSYS2).

## Build

### Linux

```bash
sudo apt install libncurses-dev          # Debian / Ubuntu
sudo dnf install ncurses-devel           # Fedora
sudo pacman -S ncurses                   # Arch

git clone https://github.com/Cyberpsych0s1s/ben.git
cd ben
make
sudo make install
```

### macOS

```bash
brew install ncurses
git clone https://github.com/Cyberpsych0s1s/ben.git
cd ben
make
sudo make install
```

### Windows (MSYS2 UCRT64)

```bash
pacman -S mingw-w64-ucrt-x86_64-gcc \
          mingw-w64-ucrt-x86_64-pdcurses \
          mingw-w64-ucrt-x86_64-make

git clone https://github.com/Cyberpsych0s1s/ben.git
cd ben
mingw32-make
```

`make install` is Unix-only. On Windows, copy `ben.exe` somewhere on your
`PATH` manually.

### Tests

```bash
make test         # or `mingw32-make test` on Windows
```

171 tests covering the gap buffer, line operations, file I/O, and undo
system.

## Usage

```bash
ben [filename]    # open a file, or start an empty buffer
```

### Normal mode

| Key       | Action                          |
| --------- | ------------------------------- |
| `h j k l` | Move left / down / up / right   |
| `i`       | Insert at cursor                |
| `a`       | Insert after cursor             |
| `A`       | Insert at end of line           |
| `o`       | Open new line below             |
| `O`       | Open new line above             |
| `x`       | Delete character under cursor   |
| `X`       | Delete character before cursor  |
| `u`       | Undo                            |
| `Ctrl+R`  | Redo                            |
| `w`       | Toggle line wrap                |
| `/`       | Search forward                  |
| `?`       | Search backward                 |
| `n`       | Next match                      |
| `N`       | Previous match                  |
| `:`       | Command mode                    |
| `Esc`     | Return to normal mode           |

### Insert mode

Type normally to insert text. `Enter` splits the line, `Backspace`/`Delete`
remove characters and merge lines at boundaries, arrow keys move the
cursor. `Esc` returns to normal mode.

### Command mode

| Command         | Action                          |
| --------------- | ------------------------------- |
| `:w`            | Save                            |
| `:w <name>`     | Save as `<name>`                |
| `:q`            | Quit                            |
| `:wq`           | Save and quit                   |
| `:wq <name>`    | Save as `<name>` and quit       |
| `:wrap`         | Enable line wrapping            |
| `:nowrap`       | Disable line wrapping           |
| `:nohl`         | Clear search highlighting       |
| `:set ic`       | Case-insensitive search         |
| `:set noic`     | Case-sensitive search           |

`:wq` will refuse to quit if the save fails — your unsaved work stays
in the buffer.

## Notes

- ben is a text editor. Files containing embedded `NUL` bytes will load
  with content truncated at the first `NUL` on each line.
- The original file's trailing-newline state is preserved on save — files
  without a final `\n` won't gain one.

## Roadmap

- [ ] Word motions (`w`, `b`, `e`, `W`, `B`, `E`)
- [ ] Line motions (`gg`, `G`, `0`, `$`)
- [ ] Delete operators (`dd`, `dw`, `d$`)
- [ ] Yank / put (`yy`, `p`, `P`)
- [ ] Config file support (`~/.benrc`)
- [ ] Visual mode
- [ ] Multiple buffers / split windows

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

## License

MIT — see [LICENSE](LICENSE).
