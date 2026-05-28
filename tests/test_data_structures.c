#include "data_structures.h"
#include "editor_state.h"
#include "gap_buffer.h"
#include "test_framework.h"
#include <string.h>

void
test_line_creation (void)
{
  // Test creating line with content
  Line *line = create_new_line ("Hello World");
  ASSERT_NOT_NULL (line, "Line should be created successfully");
  ASSERT_NOT_NULL (line->gb, "Line should have a gap buffer");
  ASSERT_EQ (11, line_get_length (line), "Line should have correct length");

  char *content = line_to_string (line);
  ASSERT_STR_EQ ("Hello World", content,
                 "Line should contain correct content");
  free (content);

  gap_buffer_destroy (line->gb);
  free (line);

  // Test creating empty line
  Line *empty_line = create_new_line_empty ();
  ASSERT_NOT_NULL (empty_line, "Empty line should be created successfully");
  ASSERT_NOT_NULL (empty_line->gb, "Empty line should have a gap buffer");
  ASSERT_EQ (0, line_get_length (empty_line),
             "Empty line should have length 0");

  gap_buffer_destroy (empty_line->gb);
  free (empty_line);
}

void
test_line_operations (void)
{
  Line *line = create_new_line ("Hello");

  // Test getting character at position
  ASSERT_EQ ('H', line_get_char_at (line, 0), "First character should be 'H'");
  ASSERT_EQ ('e', line_get_char_at (line, 1),
             "Second character should be 'e'");
  ASSERT_EQ ('\0', line_get_char_at (line, 10),
             "Character beyond line should be null");

  // Test inserting character
  line_insert_char_at (line, 5, '!');
  ASSERT_EQ (6, line_get_length (line),
             "Line length should increase after insertion");

  char *content = line_to_string (line);
  ASSERT_STR_EQ ("Hello!", content, "Line should contain inserted character");
  free (content);

  // Test inserting string
  line_insert_string_at (line, 5, " World");
  ASSERT_EQ (12, line_get_length (line),
             "Line length should increase after string insertion");

  content = line_to_string (line);
  ASSERT_STR_EQ ("Hello World!", content,
                 "Line should contain inserted string");
  free (content);

  // Test deleting character
  line_delete_char_at (line, 5);
  ASSERT_EQ (11, line_get_length (line),
             "Line length should decrease after deletion");

  content = line_to_string (line);
  ASSERT_STR_EQ ("HelloWorld!", content, "Character should be deleted");
  free (content);

  // Test deleting character before
  line_delete_char_before (line, 5);
  ASSERT_EQ (10, line_get_length (line),
             "Line length should decrease after backspace deletion");

  content = line_to_string (line);
  ASSERT_STR_EQ ("HellWorld!", content,
                 "Character should be deleted with backspace");
  free (content);

  gap_buffer_destroy (line->gb);
  free (line);
}

void
test_editor_state_initialization (void)
{
  EditorState state;

  // Test initialization without filename
  init_editor_state (&state, NULL);

  ASSERT_NOT_NULL (
      state.buffer.head,
      "Buffer should have at least one line after initialization");
  ASSERT_NOT_NULL (state.buffer.tail,
                   "Buffer tail should not be NULL after initialization");
  ASSERT_EQ (1, state.buffer.num_lines,
             "Buffer should have 1 line after initialization");
  ASSERT_NOT_NULL (state.buffer.current_line_node,
                   "Current line should not be NULL after initialization");
  ASSERT_EQ (0, state.buffer.current_col_offset,
             "Column offset should be 0 after initialization");
  ASSERT_EQ (MODE_NORMAL, state.current_mode, "Should start in normal mode");
  ASSERT_EQ (0, state.top_line, "Top line should be 0");
  ASSERT_EQ (1, state.line_wrap_enabled,
             "Line wrap should be enabled by default");
  ASSERT_NULL (state.filename, "Filename should be NULL when none provided");

  free_editor_state (&state);
}

void
test_editor_state_with_filename (void)
{
  EditorState state;
  const char *test_filename = "test_editor_state.txt";

  // Create a test file
  FILE *fp = fopen (test_filename, "w");
  if (fp)
    {
      fprintf (fp, "Line 1\nLine 2\nLine 3\n");
      fclose (fp);
    }

  // Test initialization with filename
  init_editor_state (&state, test_filename);

  ASSERT_NOT_NULL (state.buffer.head,
                   "Buffer should have lines after loading file");
  ASSERT_EQ (3, state.buffer.num_lines,
             "Buffer should have 3 lines from file");
  ASSERT_NOT_NULL (state.buffer.current_line_node,
                   "Current line should be set");
  ASSERT_EQ (state.buffer.head, state.buffer.current_line_node,
             "Current line should be first line");
  ASSERT_EQ (0, state.buffer.current_col_offset, "Column should start at 0");
  ASSERT_STR_EQ (test_filename, state.filename,
                 "Filename should be stored correctly");

  // Test line content
  char *first_line_content = line_to_string (state.buffer.head);
  ASSERT_STR_EQ ("Line 1", first_line_content,
                 "First line should contain correct content");
  free (first_line_content);

  free_editor_state (&state);
  remove (test_filename); // Clean up test file
}

void
test_line_insertion_with_editor_state (void)
{
  EditorState state;
  init_editor_state (&state, NULL);

  // The buffer should start with one empty line
  ASSERT_EQ (1, state.buffer.num_lines, "Should start with 1 line");

  // Test inserting a line after the current line
  Line *new_line = create_new_line ("Second line");
  insert_line_after (&state.buffer, state.buffer.current_line_node,
                     new_line);

  ASSERT_EQ (2, state.buffer.num_lines,
             "Buffer should have 2 lines after insertion");
  ASSERT_EQ (new_line, state.buffer.current_line_node->next,
             "New line should be after current line");
  ASSERT_EQ (state.buffer.current_line_node, new_line->prev,
             "New line should point back to current line");

  // Test inserting at end
  Line *third_line = create_new_line ("Third line");
  insert_line_at_end (&state.buffer, third_line);

  ASSERT_EQ (3, state.buffer.num_lines, "Buffer should have 3 lines");
  ASSERT_EQ (third_line, state.buffer.tail,
             "Third line should be buffer tail");

  free_editor_state (&state);
}

void
test_buffer_traversal_with_editor_state (void)
{
  EditorState state;
  init_editor_state (&state, NULL);

  // Clear the initial empty line and create test lines
  free_editor_buffer (&state.buffer);
  init_editor_buffer (&state.buffer);

  // Create a buffer with multiple lines
  Line *lines[5];
  for (int i = 0; i < 5; i++)
    {
      char content[20];
      snprintf (content, sizeof (content), "Line %d", i + 1);
      lines[i] = create_new_line (content);
      insert_line_at_end (&state.buffer, lines[i]);
    }

  // Set current line to first line
  state.buffer.current_line_node = state.buffer.head;

  // Test forward traversal
  Line *current = state.buffer.head;
  int count = 0;
  while (current != NULL)
    {
      char expected[20];
      snprintf (expected, sizeof (expected), "Line %d", count + 1);
      char *actual = line_to_string (current);

      ASSERT_STR_EQ (expected, actual,
                     "Forward traversal should visit lines in order");
      free (actual);

      current = current->next;
      count++;
    }
  ASSERT_EQ (5, count, "Forward traversal should visit all 5 lines");

  // Test backward traversal
  current = state.buffer.tail;
  count = 4;
  while (current != NULL)
    {
      char expected[20];
      snprintf (expected, sizeof (expected), "Line %d", count + 1);
      char *actual = line_to_string (current);

      ASSERT_STR_EQ (expected, actual,
                     "Backward traversal should visit lines in reverse order");
      free (actual);

      current = current->prev;
      count--;
    }
  ASSERT_EQ (-1, count, "Backward traversal should visit all 5 lines");

  free_editor_state (&state);
}

void
test_temp_message_functionality (void)
{
  EditorState state;
  init_editor_state (&state, NULL);

  // Test initial state
  ASSERT_FALSE (has_temp_message (&state),
                "Should not have temp message initially");

  // Test setting temp message
  set_temp_message (&state, "Test message");
  ASSERT_TRUE (has_temp_message (&state),
               "Should have temp message after setting");
  ASSERT_STR_EQ ("Test message", state.temp_message,
                 "Temp message should match what was set");

  // Test clearing temp message
  clear_temp_message (&state);
  ASSERT_FALSE (has_temp_message (&state),
                "Should not have temp message after clearing");
  ASSERT_EQ ('\0', state.temp_message[0],
             "Temp message buffer should be empty");

  // Test message truncation
  char long_message[300];
  memset (long_message, 'A', 299);
  long_message[299] = '\0';

  set_temp_message (&state, long_message);
  ASSERT_TRUE (has_temp_message (&state),
               "Should have temp message after setting long message");
  ASSERT_TRUE (strlen (state.temp_message) < 256,
               "Message should be truncated to buffer size");

  free_editor_state (&state);
}

void
test_line_edge_cases (void)
{
  // Test operations on NULL line
  ASSERT_EQ (0, line_get_length (NULL),
             "Getting length of NULL line should return 0");
  ASSERT_EQ ('\0', line_get_char_at (NULL, 0),
             "Getting char from NULL line should return null char");
  ASSERT_NULL (line_to_string (NULL),
               "Converting NULL line to string should return NULL");

  // Test operations at boundary positions
  Line *line = create_new_line ("test");

  // Test deleting at position 0 with backspace (should do nothing)
  line_delete_char_before (line, 0);
  ASSERT_EQ (4, line_get_length (line), "Line length should remain unchanged");

  char *content = line_to_string (line);
  ASSERT_STR_EQ ("test", content, "Line content should remain unchanged");
  free (content);

  gap_buffer_destroy (line->gb);
  free (line);
}

void
test_editor_state_edge_cases (void)
{
  EditorState state;

  // Test with NULL state
  init_editor_state (NULL, NULL);
  free_editor_state (NULL);
  set_temp_message (NULL, "test");
  clear_temp_message (NULL);
  ASSERT_FALSE (has_temp_message (NULL),
                "NULL state should not have temp message");

  // Test with valid state but NULL message
  init_editor_state (&state, NULL);
  set_temp_message (&state, NULL);
  ASSERT_FALSE (has_temp_message (&state),
                "Should not have temp message when setting NULL");

  free_editor_state (&state);
}

void
run_data_structures_tests (void)
{
  TEST_SUITE_START ("Data Structures Tests");

  test_line_creation ();
  test_line_operations ();
  test_editor_state_initialization ();
  test_editor_state_with_filename ();
  test_line_insertion_with_editor_state ();
  test_buffer_traversal_with_editor_state ();
  test_temp_message_functionality ();
  test_line_edge_cases ();
  test_editor_state_edge_cases ();

  TEST_SUITE_END ("Data Structures Tests");
}
