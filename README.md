# Ben: A Vim-like Text Editor

A minimalist, Vim-inspired text editor built in C with ncurses.

## Installation

### Dependencies
Install ncurses development libraries:

**Ubuntu/Debian:**
```bash
sudo apt update && sudo apt install libncurses-dev
```

**Fedora:**
```bash
sudo dnf install ncurses-devel
```

**Arch Linux:**
```bash
sudo pacman -S ncurses
```

### Build & Install
```bash
git clone https://github.com/Almahr1/ben.git
cd ben
make
sudo make install
```

## Usage

### Starting
```bash
ben [filename]    # Open file or create new
```

### Normal Mode
| Command | Action |
|---------|--------|
| `h j k l` | Move left/down/up/right |
| `i` | Insert mode at cursor |
| `a` | Insert mode after cursor |
| `A` | Insert mode at end of line |
| `o` | Insert new line below |
| `O` | Insert new line above |
| `x` | Delete character under cursor |
| `X` | Delete character before cursor |
| `u` | Undo |
| `Ctrl+R` | Redo |
| `w` | Toggle line wrapping |
| `/` | Search forward |
| `?` | Search backward |
| `n` | Next search result |
| `N` | Previous search result |
| `:` | Command mode |
| `Esc` | Return to normal mode |

### Insert Mode
- Type normally to insert text
- `Enter` splits line
- `Backspace`/`Delete` removes characters
- Arrow keys move cursor
- `Esc` returns to normal mode

### Command Mode
| Command | Action |
|---------|--------|
| `:w` | Save file |
| `:w filename` | Save as filename |
| `:q` | Quit |
| `:wq` | Save and quit |
| `:wrap` | Enable line wrapping |
| `:nowrap` | Disable line wrapping |
| `:nohl` | Clear search highlighting |
| `:set ic` | Case insensitive search |
| `:set noic` | Case sensitive search |

## Bounty Board
- [] Add config file support
- [] Add word motions 

## License

MIT License - see LICENSE file for details.
