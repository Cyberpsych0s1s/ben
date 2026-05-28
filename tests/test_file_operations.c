// SPDX-License-Identifier: MIT
#include "data_structures.h"
#include "editor_state.h"
#include "test_framework.h"
#include "text_editor_functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// A global filename for testing
const char *TEST_FILENAME = "test_file.txt";

void
test_save_and_load_file_with_editor_state (void)
{
  TEST_CASE_START (
      "saveToFile and loadFromFile with EditorState - multiple lines");

  // Initialize editor state and populate with test content
  EditorState state;
  init_editor_state (&state, NULL);

  // Clear the default empty line and add our test content
  free_editor_buffer (&state.buffer);
  init_editor_buffer (&state.buffer);

  Line *line1 = create_new_line ("Hello world!");
  insert_line_at_end (&state.buffer, line1);
  Line *line2 = create_new_line ("This is a test file.");
  insert_line_at_end (&state.buffer, line2);
  Line *line3 = create_new_line ("");
  insert_line_at_end (&state.buffer, line3);
  Line *line4 = create_new_line ("The end.");
  insert_line_at_end (&state.buffer, line4);

  // Set current line
  state.buffer.current_line_node = line1;
  state.buffer.current_col_offset = 0;

  // Save the buffer to a file
  saveToFile (TEST_FILENAME, &state.buffer, NULL, NULL);

  // Free the current state and create a new one for loading
  free_editor_state (&state);

  // Initialize new state and load from file
  EditorState load_state;
  init_editor_state (&load_state, TEST_FILENAME);

  // Assert that the content and number of lines are correct
  ASSERT_EQ (4, load_state.buffer.num_lines,
             "Loaded buffer should have 4 lines");
  ASSERT_NOT_NULL (load_state.buffer.current_line_node,
                   "Current line should be set after loading");
  ASSERT_EQ (load_state.buffer.head, load_state.buffer.current_line_node,
             "Current line should be first line");
  ASSERT_EQ (0, load_state.buffer.current_col_offset,
             "Column offset should be 0 after loading");

  Line *current_line = load_state.buffer.head;
  char *content1 = line_to_string (current_line);
  ASSERT_STR_EQ ("Hello world!", content1, "First line content should match");
  free (content1);
  current_line = current_line->next;

  char *content2 = line_to_string (current_line);
  ASSERT_STR_EQ ("This is a test file.", content2,
                 "Second line content should match");
  free (content2);
  current_line = current_line->next;

  char *content3 = line_to_string (current_line);
  ASSERT_STR_EQ ("", content3, "Third line should be empty");
  free (content3);
  current_line = current_line->next;

  char *content4 = line_to_string (current_line);
  ASSERT_STR_EQ ("The end.", content4, "Fourth line content should match");
  free (content4);

  // Clean up
  free_editor_state (&load_state);
  remove (TEST_FILENAME);
  TEST_CASE_END ();
}

void
test_load_empty_file_with_editor_state (void)
{
  TEST_CASE_START ("loadFromFile with EditorState - empty file");

  // Create an empty file
  FILE *fp = fopen (TEST_FILENAME, "w");
  if (fp)
    fclose (fp);

  // Initialize editor state with empty file
  EditorState state;
  init_editor_state (&state, TEST_FILENAME);

  // Assert that a single, empty line is created
  ASSERT_EQ (1, state.buffer.num_lines,
             "Loaded buffer should have 1 line for an empty file");
  ASSERT_NOT_NULL (state.buffer.head, "Head should not be NULL");
  ASSERT_NOT_NULL (state.buffer.current_line_node,
                   "Current line node should not be NULL");
  ASSERT_EQ (state.buffer.head, state.buffer.current_line_node,
             "Current line should be the head");
  ASSERT_EQ (0, state.buffer.current_col_offset, "Column offset should be 0");

  char *content = line_to_string (state.buffer.head);
  ASSERT_STR_EQ ("", content, "The line should be empty");
  free (content);

  // Test filename storage
  ASSERT_STR_EQ (TEST_FILENAME, state.filename,
                 "Filename should be stored correctly");

  // Clean up
  free_editor_state (&state);
  remove (TEST_FILENAME);
  TEST_CASE_END ();
}

void
test_load_nonexistent_file_with_editor_state (void)
{
  TEST_CASE_START ("loadFromFile with EditorState - nonexistent file");

  const char *nonexistent_file = "nonexistent_file_12345.txt";

  // Initialize editor state with nonexistent file
  EditorState state;
  init_editor_state (&state, nonexistent_file);

  // Should create a buffer with one empty line when file doesn't exist
  ASSERT_EQ (1, state.buffer.num_lines,
             "Should have 1 empty line when file doesn't exist");
  ASSERT_NOT_NULL (state.buffer.head, "Head should not be NULL");
  ASSERT_NOT_NULL (state.buffer.current_line_node,
                   "Current line node should not be NULL");
  ASSERT_EQ (0, state.buffer.current_col_offset, "Column offset should be 0");

  char *content = line_to_string (state.buffer.head);
  ASSERT_STR_EQ ("", content, "The line should be empty");
  free (content);

  // Test filename storage
  ASSERT_STR_EQ (nonexistent_file, state.filename,
                 "Filename should be stored even if file doesn't exist");

  // Clean up
  free_editor_state (&state);
  TEST_CASE_END ();
}

void
test_save_to_new_file_with_editor_state (void)
{
  TEST_CASE_START ("saveToFile with EditorState - save to new file");

  // Create editor state with content
  EditorState state;
  init_editor_state (&state, NULL);

  // Replace default empty line with test content
  Line *new_line = create_new_line ("Test content for new file");
  line_insert_string_at (state.buffer.current_line_node, 0,
                         "Test content for new file");

  // Save to a new file
  const char *new_filename = "new_test_file.txt";
  saveToFile (new_filename, &state.buffer, NULL, NULL);

  // Verify file was created and contains correct content
  FILE *fp = fopen (new_filename, "r");
  ASSERT_NOT_NULL (fp, "New file should be created");

  if (fp)
    {
      char buffer[256];
      char *result = fgets (buffer, sizeof (buffer), fp);
      ASSERT_NOT_NULL (result, "Should be able to read from new file");

      // Remove newline if present
      if (result)
        {
          size_t len = strlen (result);
          if (len > 0 && result[len - 1] == '\n')
            {
              result[len - 1] = '\0';
            }
          ASSERT_STR_EQ ("Test content for new file", result,
                         "File content should match");
        }
      fclose (fp);
    }

  // Clean up
  free_editor_state (&state);
  remove (new_filename);
  TEST_CASE_END ();
}

void
test_save_with_multiple_modes (void)
{
  TEST_CASE_START ("File operations with different editor modes");

  EditorState state;
  init_editor_state (&state, NULL);

  // Test saving in different modes
  state.current_mode = MODE_INSERT;
  saveToFile (TEST_FILENAME, &state.buffer, NULL, NULL);

  state.current_mode = MODE_COMMAND;
  saveToFile (TEST_FILENAME, &state.buffer, NULL, NULL);

  state.current_mode = MODE_NORMAL;
  saveToFile (TEST_FILENAME, &state.buffer, NULL, NULL);

  // Verify file exists and is readable
  FILE *fp = fopen (TEST_FILENAME, "r");
  ASSERT_NOT_NULL (fp, "File should be saved regardless of editor mode");
  if (fp)
    fclose (fp);

  // Clean up
  free_editor_state (&state);
  remove (TEST_FILENAME);
  TEST_CASE_END ();
}

void
test_file_operations_with_line_wrap_settings (void)
{
  TEST_CASE_START ("File operations preserve line wrap settings");

  EditorState state;
  init_editor_state (&state, NULL);

  // Test with line wrap disabled
  state.line_wrap_enabled = 0;

  // Add some long content
  Line *long_line = create_new_line (
      "This is a very long line that would normally wrap around in the editor "
      "but should be saved as a single line in the file");
  free_editor_buffer (&state.buffer);
  init_editor_buffer (&state.buffer);
  insert_line_at_end (&state.buffer, long_line);
  state.buffer.current_line_node = long_line;

  saveToFile (TEST_FILENAME, &state.buffer, NULL, NULL);

  // Load into new state
  EditorState load_state;
  init_editor_state (&load_state, TEST_FILENAME);

  // Verify content
  ASSERT_EQ (1, load_state.buffer.num_lines,
             "Should have 1 line regardless of wrap setting");
  char *content = line_to_string (load_state.buffer.head);
  ASSERT_STR_EQ ("This is a very long line that would normally wrap around in "
                 "the editor but should be saved as a single line in the file",
                 content, "Long line should be preserved");
  free (content);

  // Clean up
  free_editor_state (&state);
  free_editor_state (&load_state);
  remove (TEST_FILENAME);
  TEST_CASE_END ();
}

void
run_file_operations_tests (void)
{
  TEST_SUITE_START ("File Operations Tests with EditorState");

  test_save_and_load_file_with_editor_state ();
  test_load_empty_file_with_editor_state ();
  test_load_nonexistent_file_with_editor_state ();
  test_save_to_new_file_with_editor_state ();
  test_save_with_multiple_modes ();
  test_file_operations_with_line_wrap_settings ();

  TEST_SUITE_END ("File Operations Tests with EditorState");
}
