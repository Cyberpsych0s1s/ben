// SPDX-License-Identifier: MIT
#include "gap_buffer.h"
#include "test_framework.h"

void
test_gap_buffer_creation (void)
{
  // Test normal creation
  GapBuffer *gb = gap_buffer_create (32);
  ASSERT_NOT_NULL (gb, "Gap buffer should be created successfully");
  ASSERT_EQ (32, gb->capacity, "Gap buffer should have correct capacity");
  ASSERT_EQ (0, gb->gap_start, "Gap should start at position 0");
  ASSERT_EQ (32, gb->gap_end, "Gap should end at capacity");
  ASSERT_EQ (0, gap_buffer_length (gb), "Empty buffer should have length 0");
  gap_buffer_destroy (gb);

  // Test minimum capacity enforcement
  gb = gap_buffer_create (5); // Less than MIN_GAP_SIZE (16)
  ASSERT_NOT_NULL (gb,
                   "Gap buffer with small capacity should still be created");
  ASSERT_TRUE (gb->capacity >= 16,
               "Gap buffer should enforce minimum capacity");
  gap_buffer_destroy (gb);
}

void
test_gap_buffer_insert_char (void)
{
  GapBuffer *gb = gap_buffer_create (16);

  // Test single character insertion
  gap_buffer_insert_char (gb, 'a');
  ASSERT_EQ (1, gap_buffer_length (gb),
             "Buffer length should be 1 after inserting one char");
  ASSERT_EQ ('a', gap_buffer_get_char_at (gb, 0),
             "First character should be 'a'");

  // Test multiple character insertion
  gap_buffer_insert_char (gb, 'b');
  gap_buffer_insert_char (gb, 'c');
  ASSERT_EQ (3, gap_buffer_length (gb),
             "Buffer length should be 3 after inserting three chars");
  ASSERT_EQ ('b', gap_buffer_get_char_at (gb, 1),
             "Second character should be 'b'");
  ASSERT_EQ ('c', gap_buffer_get_char_at (gb, 2),
             "Third character should be 'c'");

  gap_buffer_destroy (gb);
}

void
test_gap_buffer_insert_string (void)
{
  GapBuffer *gb = gap_buffer_create (16);

  // Test string insertion
  gap_buffer_insert_string (gb, "hello");
  ASSERT_EQ (5, gap_buffer_length (gb),
             "Buffer length should be 5 after inserting 'hello'");

  char *result = gap_buffer_to_string (gb);
  ASSERT_STR_EQ ("hello", result, "Buffer content should be 'hello'");
  free (result);

  // Test inserting empty string
  gap_buffer_insert_string (gb, "");
  ASSERT_EQ (5, gap_buffer_length (gb),
             "Buffer length should remain 5 after inserting empty string");

  // Test inserting NULL
  gap_buffer_insert_string (gb, NULL);
  ASSERT_EQ (5, gap_buffer_length (gb),
             "Buffer length should remain 5 after inserting NULL");

  gap_buffer_destroy (gb);
}

void
test_gap_buffer_cursor_movement (void)
{
  GapBuffer *gb = gap_buffer_create (16);
  gap_buffer_insert_string (gb, "hello");

  // Test moving cursor to different positions
  gap_buffer_move_cursor_to (gb, 3);
  ASSERT_EQ (3, gap_buffer_cursor_position (gb),
             "Cursor should be at position 3");

  gap_buffer_move_cursor_to (gb, 0);
  ASSERT_EQ (0, gap_buffer_cursor_position (gb),
             "Cursor should be at position 0");

  gap_buffer_move_cursor_to (gb, 5);
  ASSERT_EQ (5, gap_buffer_cursor_position (gb),
             "Cursor should be at position 5");

  // Test moving cursor beyond buffer length
  gap_buffer_move_cursor_to (gb, 10);
  ASSERT_EQ (5, gap_buffer_cursor_position (gb),
             "Cursor should be clamped to buffer length");

  gap_buffer_destroy (gb);
}

void
test_gap_buffer_deletion (void)
{
  GapBuffer *gb = gap_buffer_create (16);
  gap_buffer_insert_string (gb, "hello");

  // Test deleting character at cursor
  gap_buffer_move_cursor_to (gb, 1);
  gap_buffer_delete_char (gb);
  ASSERT_EQ (4, gap_buffer_length (gb),
             "Buffer length should be 4 after deletion");

  char *result = gap_buffer_to_string (gb);
  ASSERT_STR_EQ ("hllo", result,
                 "Buffer should contain 'hllo' after deleting 'e'");
  free (result);

  // Test deleting character before cursor
  gap_buffer_move_cursor_to (gb, 2);
  gap_buffer_delete_char_before (gb);
  ASSERT_EQ (3, gap_buffer_length (gb),
             "Buffer length should be 3 after backspace deletion");

  result = gap_buffer_to_string (gb);
  ASSERT_STR_EQ ("hlo", result,
                 "Buffer should contain 'hlo' after deleting 'l'");
  free (result);

  gap_buffer_destroy (gb);
}

void
test_gap_buffer_complex_editing (void)
{
  GapBuffer *gb = gap_buffer_create (16);

  // Insert initial text
  gap_buffer_insert_string (gb, "Hello World");

  // Move cursor and insert
  gap_buffer_move_cursor_to (gb, 6);
  gap_buffer_insert_string (gb, "Beautiful ");

  char *result = gap_buffer_to_string (gb);
  ASSERT_STR_EQ ("Hello Beautiful World", result,
                 "Buffer should contain modified text");
  free (result);

  // Move cursor and delete
  gap_buffer_move_cursor_to (gb, 6);
  for (int i = 0; i < 10; i++)
    {
      gap_buffer_delete_char (gb);
    }

  result = gap_buffer_to_string (gb);
  ASSERT_STR_EQ ("Hello World", result,
                 "Buffer should be back to original after deletion");
  free (result);

  gap_buffer_destroy (gb);
}

void
test_gap_buffer_capacity_expansion (void)
{
  GapBuffer *gb = gap_buffer_create (16);

  // Fill buffer beyond initial capacity
  for (int i = 0; i < 50; i++)
    {
      gap_buffer_insert_char (gb, 'a' + (i % 26));
    }

  ASSERT_EQ (50, gap_buffer_length (gb),
             "Buffer should contain 50 characters");
  ASSERT_TRUE (gb->capacity > 16, "Buffer capacity should have expanded");

  // Verify content integrity by converting to string
  char *result = gap_buffer_to_string (gb);
  char expected[51];
  for (int i = 0; i < 50; i++)
    {
      expected[i] = 'a' + (i % 26);
    }
  expected[50] = '\0';
  ASSERT_STR_EQ (expected, result,
                 "All characters preserved after capacity expansion");
  free (result);

  gap_buffer_destroy (gb);
}

void
test_gap_buffer_edge_cases (void)
{
  GapBuffer *gb = gap_buffer_create (16);

  // Test operations on empty buffer
  ASSERT_EQ ('\0', gap_buffer_get_char_at (gb, 0),
             "Getting char from empty buffer should return null char");
  gap_buffer_delete_char (gb);
  gap_buffer_delete_char_before (gb);
  ASSERT_EQ (0, gap_buffer_length (gb),
             "Buffer should remain empty after invalid deletions");

  // Test getting character beyond buffer
  gap_buffer_insert_string (gb, "test");
  ASSERT_EQ ('\0', gap_buffer_get_char_at (gb, 10),
             "Getting char beyond buffer should return null char");

  // Test cursor at end of buffer
  gap_buffer_move_cursor_to (gb, 4);
  gap_buffer_delete_char (gb);
  ASSERT_EQ (4, gap_buffer_length (gb),
             "Buffer length should remain unchanged when deleting at end");

  gap_buffer_destroy (gb);
}

void
run_gap_buffer_tests (void)
{
  TEST_SUITE_START ("Gap Buffer Tests");

  test_gap_buffer_creation ();
  test_gap_buffer_insert_char ();
  test_gap_buffer_insert_string ();
  test_gap_buffer_cursor_movement ();
  test_gap_buffer_deletion ();
  test_gap_buffer_complex_editing ();
  test_gap_buffer_capacity_expansion ();
  test_gap_buffer_edge_cases ();

  TEST_SUITE_END ("Gap Buffer Tests");
}
