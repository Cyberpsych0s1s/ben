#include "data_structures.h"
#include "test_framework.h"
#include "undo.h"

void
test_undo_insert_char (void)
{
  TextBuffer buffer;
  init_editor_buffer (&buffer);
  init_undo_system ();

  Line *line = create_new_line ("hello");
  insert_line_at_end (&buffer, line);
  buffer.current_line_node = line;

  push_undo_operation (UNDO_INSERT_CHAR, line, 5, "!", 1);
  line_insert_char_at (line, 5, '!');

  ASSERT_TRUE (can_undo (), "Should Be Able To Undo");
  perform_undo (&buffer);

  char *undone_content = line_to_string (line);
  ASSERT_STR_EQ ("hello", undone_content,
                 "Undo Should Restore Un-Modified Content");

  ASSERT_TRUE (can_redo (), "Should Be Able to Redo");
  perform_redo (&buffer);

  char *redone_content = line_to_string (line);
  ASSERT_STR_EQ ("hello!", redone_content,
                 "Redo Should Restore Modified Content");

  free (undone_content);
  free (redone_content);
  free_editor_buffer (&buffer);
}

void
test_undo_delete_char (void)
{
  TextBuffer buffer;
  init_editor_buffer (&buffer);
  init_undo_system ();

  Line *line = create_new_line ("Testing");
  insert_line_at_end (&buffer, line);
  buffer.current_line_node = line;

  push_undo_operation (UNDO_DELETE_CHAR, line, 6, "g", 1);
  line_delete_char_at (line, 6);

  ASSERT_TRUE (can_undo (), "Should Be Able To Undo");
  perform_undo (&buffer);

  char *undone_content = line_to_string (line);
  ASSERT_STR_EQ ("Testing", undone_content,
                 "Undo Should Restore Un-Modified Content");

  ASSERT_TRUE (can_redo (), "Should Be Able to Redo");
  perform_redo (&buffer);

  char *redone_content = line_to_string (line);
  ASSERT_STR_EQ ("Testin", redone_content,
                 "Redo Should Restore Modified Content");

  free (undone_content);
  free (redone_content);
  free_editor_buffer (&buffer);
}

void
test_undo_split_line (void)
{
  TextBuffer buffer;
  init_editor_buffer (&buffer);
  init_undo_system ();

  Line *line = create_new_line ("Hello World");
  insert_line_at_end (&buffer, line);
  buffer.current_line_node = line;

  char *second_part = "World";
  push_undo_operation (UNDO_SPLIT_LINE, line, 6, second_part,
                       strlen (second_part));

  Line *new_line = create_new_line ("World");

  gap_buffer_move_cursor_to (line->gb, 6);
  size_t chars_to_delete = line_get_length (line) - 6;
  for (size_t i = 0; i < chars_to_delete; i++)
    {
      gap_buffer_delete_char (line->gb);
    }

  insert_line_after (&buffer, line, new_line);

  ASSERT_EQ (2, buffer.num_lines, "Should Have 2 Lines After Split");

  char *first_line_content = line_to_string (line);
  char *second_line_content = line_to_string (new_line);
  ASSERT_STR_EQ ("Hello ", first_line_content,
                 "First Line Should Have Correct Content");
  ASSERT_STR_EQ ("World", second_line_content,
                 "Second Line Should Have Correct Content");

  ASSERT_TRUE (can_undo (), "Should Be Able To Undo");
  perform_undo (&buffer);

  ASSERT_EQ (1, buffer.num_lines, "Should Have 1 Line After Undo");
  char *merged_content = line_to_string (line);
  ASSERT_STR_EQ ("Hello World", merged_content,
                 "Undo Should Merge Lines Back");

  ASSERT_TRUE (can_redo (), "Should Be Able To Redo");
  perform_redo (&buffer);
  ASSERT_EQ (2, buffer.num_lines, "Should Have 2 Lines After Redo");

  free (first_line_content);
  free (second_line_content);
  free (merged_content);
  free_editor_buffer (&buffer);
}

void
test_undo_merge_lines (void)
{
  TextBuffer buffer;
  init_editor_buffer (&buffer);
  init_undo_system ();

  Line *first_line = create_new_line ("Hello ");
  Line *second_line = create_new_line ("World");
  insert_line_at_end (&buffer, first_line);
  insert_line_at_end (&buffer, second_line);
  buffer.current_line_node = first_line;

  char *second_content = line_to_string (second_line);
  push_undo_operation (UNDO_MERGE_LINES, first_line,
                       line_get_length (first_line), second_content,
                       strlen (second_content));

  line_insert_string_at (first_line, line_get_length (first_line),
                         second_content);

  first_line->next = second_line->next;
  if (second_line->next)
    {
      second_line->next->prev = first_line;
    }
  else
    {
      buffer.tail = first_line;
    }
  gap_buffer_destroy (second_line->gb);
  free (second_line);
  buffer.num_lines--;

  ASSERT_EQ (1, buffer.num_lines, "Should Have 1 Line After Merge");

  char *merged_content = line_to_string (first_line);
  ASSERT_STR_EQ ("Hello World", merged_content,
                 "Merged Line Should Have Combined Content");

  ASSERT_TRUE (can_undo (), "Should Be Able To Undo");
  perform_undo (&buffer);

  ASSERT_EQ (2, buffer.num_lines, "Should Have 2 Lines After Undo");

  char *first_undone = line_to_string (first_line);
  char *second_undone = line_to_string (first_line->next);
  ASSERT_STR_EQ ("Hello ", first_undone, "First Line Should Be Restored");
  ASSERT_STR_EQ ("World", second_undone, "Second Line Should Be Restored");

  ASSERT_TRUE (can_redo (), "Should Be Able To Redo");
  perform_redo (&buffer);
  ASSERT_EQ (1, buffer.num_lines, "Should Have 1 Line After Redo");

  free (second_content);
  free (merged_content);
  free (first_undone);
  free (second_undone);
  free_editor_buffer (&buffer);
}

void
test_undo_complex_scenarios (void)
{
  TextBuffer buffer;
  init_editor_buffer (&buffer);
  init_undo_system ();

  Line *line = create_new_line ("test");
  insert_line_at_end (&buffer, line);
  buffer.current_line_node = line;

  push_undo_operation (UNDO_INSERT_CHAR, line, 4, "!", 1);
  line_insert_char_at (line, 4, '!');

  Line *new_line = create_new_line ("second");
  push_undo_operation (UNDO_INSERT_LINE, line, 0, "", 0);
  insert_line_after (&buffer, line, new_line);

  push_undo_operation (UNDO_DELETE_CHAR, line, 4, "!", 1);
  line_delete_char_at (line, 4);

  ASSERT_EQ (2, buffer.num_lines, "Should Have 2 Lines After All Operations");

  char *final_content = line_to_string (line);
  ASSERT_STR_EQ ("test", final_content,
                 "First Line Should Be Back To Original");

  ASSERT_TRUE (can_undo (), "Should Be Able To Undo Delete");
  perform_undo (&buffer);

  char *after_first_undo = line_to_string (line);
  ASSERT_STR_EQ ("test!", after_first_undo,
                 "Should Restore Deleted Character");

  ASSERT_TRUE (can_undo (), "Should Be Able To Undo Insert Line");
  perform_undo (&buffer);
  ASSERT_EQ (1, buffer.num_lines, "Should Have 1 Line After Second Undo");

  ASSERT_TRUE (can_undo (), "Should Be Able To Undo Insert Char");
  perform_undo (&buffer);

  char *original_content = line_to_string (line);
  ASSERT_STR_EQ ("test", original_content, "Should Be Back To Original State");

  ASSERT_TRUE (can_redo (), "Should Be Able To Redo Insert Char");
  perform_redo (&buffer);

  ASSERT_TRUE (can_redo (), "Should Be Able To Redo Insert Line");
  perform_redo (&buffer);
  ASSERT_EQ (2, buffer.num_lines, "Should Have 2 Lines After Redo");

  free (final_content);
  free (after_first_undo);
  free (original_content);
  free_editor_buffer (&buffer);
}

void
test_undo_edge_cases (void)
{
  TextBuffer buffer;
  init_editor_buffer (&buffer);
  init_undo_system ();

  ASSERT_FALSE (can_undo (), "Should Not Be Able To Undo On Empty Buffer");
  ASSERT_FALSE (can_redo (), "Should Not Be Able To Redo On Empty Buffer");

  Line *line = create_new_line ("test line");
  insert_line_at_end (&buffer, line);
  buffer.current_line_node = line;

  push_undo_operation (UNDO_INSERT_CHAR, line, 0, "x", 1);

  invalidate_undo_operations_for_line (line);

  perform_undo (&buffer);

  char *content = line_to_string (line);
  ASSERT_STR_EQ ("test line", content,
                 "Buffer Should Remain Unchanged With Invalid Undo");

  Line *empty_line = create_new_line ("");
  insert_line_at_end (&buffer, empty_line);

  push_undo_operation (UNDO_INSERT_CHAR, line, 1000, "z", 1);
  perform_undo (&buffer);

  char *final_content = line_to_string (line);
  ASSERT_STR_EQ ("test line", final_content,
                 "Line Should Handle Invalid Column Position");

  free (content);
  free (final_content);
  free_editor_buffer (&buffer);
}

// Test case simulating using the editor instead of manual manipulation
void
test_undo_insert_line_with_editor_functions (void)
{
  TextBuffer buffer;
  init_editor_buffer (&buffer);
  init_undo_system ();

  // Create initial empty line (like the editor would)
  Line *initial_line = create_new_line ("");
  insert_line_at_end (&buffer, initial_line);
  buffer.current_line_node = initial_line;

  ASSERT_EQ (1, buffer.num_lines, "Should start with 1 line");

  // Simulate the 'o' command (insert line below)
  push_undo_operation (UNDO_INSERT_LINE, initial_line, 0, "", 0);
  Line *new_line = create_new_line ("New line content");
  insert_line_after (&buffer, initial_line, new_line);
  buffer.current_line_node = new_line;

  ASSERT_EQ (2, buffer.num_lines, "Should have 2 lines after insert");

  // Test undo
  ASSERT_TRUE (can_undo (), "Should be able to undo");
  perform_undo (&buffer);
  ASSERT_EQ (1, buffer.num_lines, "Should have 1 line after undo");

  // Test redo
  ASSERT_TRUE (can_redo (), "Should be able to redo");
  perform_redo (&buffer);
  ASSERT_EQ (2, buffer.num_lines, "Should have 2 lines after redo");

  free_editor_buffer (&buffer);
}

void
run_undo_tests (void)
{
  TEST_SUITE_START ("Undo System Tests");

  test_undo_insert_char ();
  test_undo_delete_char ();
  test_undo_split_line ();
  test_undo_merge_lines ();
  test_undo_complex_scenarios ();
  test_undo_edge_cases ();
  test_undo_insert_line_with_editor_functions ();

  TEST_SUITE_END ("Undo System Tests");
}
