# Contributing to ben

Thanks for your interest! ben is a small, focused project — the goal is a
readable, hackable Vim-inspired editor, not a kitchen-sink IDE.

## Getting set up

Follow the build instructions in the [README](README.md). The full test
suite must pass before any change is merged:

```bash
make test    # or `mingw32-make test` on Windows
```

## What's in scope

- Bug fixes and crash hardening.
- Items from the [Roadmap](README.md#roadmap).
- Performance improvements with measured numbers.
- Tests for behaviour that isn't covered yet.
- Portability fixes (Linux, macOS, Windows / MSYS2).

## What's out of scope

- Plugins, scripting languages, embedded interpreters.
- Syntax highlighting (would be a significant rewrite of the render path).
- GUI front-ends — ben is and stays a terminal editor.

## Code style

- GNU C style (the existing codebase is formatted with `clang-format`'s
  GNU profile). Two-space indents, function name on its own line.
- C99 with one POSIX function (`rename`); platform-specific code goes
  behind `#ifdef _WIN32`.
- Prefer adding a test alongside a bug fix.
- No external dependencies beyond the curses library.

## Pull requests

- One logical change per PR. Bug fix, feature, refactor — separately.
- Update the [CHANGELOG](CHANGELOG.md) under `## [Unreleased]`.
- Make sure `make test` passes.
- A short rationale in the PR body is enough — no template required.

## Reporting bugs

Open an issue with:

- Your OS, terminal emulator, and compiler version.
- Steps to reproduce.
- What you expected vs. what happened.
- A minimal sample file if the bug depends on content.

## License

By contributing you agree your code is released under the MIT license.
