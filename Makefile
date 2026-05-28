# OS detection — works in MSYS2 shell *and* native PowerShell/CMD.
# $(OS) is set by Windows itself; on Unix it's empty and we fall back to uname.
ifeq ($(OS),Windows_NT)
    DETECTED_OS = Windows_NT
    LDFLAGS     = -lpdcurses
    EXE_SUFFIX  = .exe
    RM          = del /f /q
    FIXPATH     = $(subst /,\,$1)
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
        DETECTED_OS = Darwin
    else
        DETECTED_OS = Linux
    endif
    LDFLAGS    = -lncurses
    EXE_SUFFIX =
    RM         = rm -f
    FIXPATH    = $1
endif

CC      = gcc
TARGET  = ben$(EXE_SUFFIX)
TEST_TARGET = ben_tests$(EXE_SUFFIX)

CFLAGS = -Wall -Wextra -std=c11 -g -Iinclude -Itests

# Main application sources
SRCS_MAIN = src/bin.c src/color_config.c src/file_operations.c \
            src/text_editor_functions.c src/gap_buffer.c src/undo.c \
            src/editor_state.c src/search.c
OBJS_MAIN = $(SRCS_MAIN:.c=.o)

# Shared library sources used by both main and tests
LIB_SRCS = src/color_config.c src/file_operations.c \
           src/text_editor_functions.c src/gap_buffer.c src/undo.c \
           src/editor_state.c src/search.c
LIB_OBJS = $(LIB_SRCS:.c=.o)

# Test sources
TEST_SRCS = tests/test_runner.c tests/test_framework.c \
            tests/test_gap_buffer.c tests/test_data_structures.c \
            tests/test_file_operations.c tests/test_undo.c
TEST_OBJS = $(TEST_SRCS:.c=.o)

# Everything linked into the test executable
TEST_EXEC_OBJS = $(LIB_OBJS) $(TEST_OBJS)

INSTALL_DIR = /usr/local/bin

.PHONY: all clean install deps test

all: $(TARGET)

$(TARGET): $(OBJS_MAIN)
	$(CC) $(OBJS_MAIN) -o $(TARGET) $(LDFLAGS)

$(TEST_TARGET): $(TEST_EXEC_OBJS)
	$(CC) $(TEST_EXEC_OBJS) -o $(TEST_TARGET) $(LDFLAGS)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

ifeq ($(DETECTED_OS),Windows_NT)
clean:
	-$(RM) $(call FIXPATH,$(OBJS_MAIN)) 2>NUL
	-$(RM) $(call FIXPATH,$(TEST_OBJS)) 2>NUL
	-$(RM) $(TARGET) $(TEST_TARGET) 2>NUL
else
clean:
	$(RM) $(OBJS_MAIN) $(TEST_OBJS) $(TARGET) $(TEST_TARGET)
endif

install: $(TARGET)
ifeq ($(DETECTED_OS),Windows_NT)
	@echo "On Windows, copy $(TARGET) somewhere on your PATH manually."
else
	sudo cp $(TARGET) $(INSTALL_DIR)
	@echo "ben installed to $(INSTALL_DIR)."
endif

deps:
ifeq ($(DETECTED_OS),Linux)
	sudo apt-get update
	sudo apt-get install libncurses5-dev libncursesw5-dev
endif
ifeq ($(DETECTED_OS),Darwin)
	brew install ncurses
endif
ifeq ($(DETECTED_OS),Windows_NT)
	@echo "On Windows, install pdcurses via:"
	@echo "  pacman -S mingw-w64-ucrt-x86_64-pdcurses mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make"
endif
