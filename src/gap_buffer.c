// SPDX-License-Identifier: MIT
#include "gap_buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN_GAP_SIZE 16
#define GROWTH_FACTOR 2

GapBuffer *
gap_buffer_create (size_t initial_capacity)
{
  if (initial_capacity < MIN_GAP_SIZE)
    {
      initial_capacity = MIN_GAP_SIZE;
    }

  GapBuffer *gb = malloc (sizeof (GapBuffer));
  if (!gb)
    return NULL;

  gb->buffer = malloc (initial_capacity);
  if (!gb->buffer)
    {
      free (gb);
      return NULL;
    }

  gb->capacity = initial_capacity;
  gb->gap_start = 0;
  gb->gap_end = initial_capacity;

  return gb;
}

void
gap_buffer_destroy (GapBuffer *gb)
{
  if (gb)
    {
      free (gb->buffer);
      free (gb);
    }
}

size_t
gap_buffer_gap_size (const GapBuffer *gb)
{
  return gb->gap_end - gb->gap_start;
}

size_t
gap_buffer_length (const GapBuffer *gb)
{
  return gb->capacity - gap_buffer_gap_size (gb);
}

size_t
gap_buffer_cursor_position (const GapBuffer *gb)
{
  return gb->gap_start;
}

int
gap_buffer_ensure_capacity (GapBuffer *gb, size_t needed_capacity)
{
  if (gb->capacity >= needed_capacity)
    return 0;

  size_t new_capacity = gb->capacity * GROWTH_FACTOR;
  if (new_capacity < needed_capacity)
    {
      new_capacity = needed_capacity;
    }

  char *new_buffer = malloc (new_capacity);
  if (!new_buffer)
    return -1;

  memcpy (new_buffer, gb->buffer, gb->gap_start);

  size_t after_gap_size = gb->capacity - gb->gap_end;
  size_t new_gap_end = new_capacity - after_gap_size;
  memcpy (new_buffer + new_gap_end, gb->buffer + gb->gap_end, after_gap_size);

  free (gb->buffer);
  gb->buffer = new_buffer;
  gb->gap_end = new_gap_end;
  gb->capacity = new_capacity;
  return 0;
}

void
gap_buffer_move_cursor_to (GapBuffer *gb, size_t position)
{
  if (position > gap_buffer_length (gb))
    {
      position = gap_buffer_length (gb);
    }

  if (position < gb->gap_start)
    {
      size_t move_size = gb->gap_start - position;
      memmove (gb->buffer + gb->gap_end - move_size, gb->buffer + position,
               move_size);
      gb->gap_start = position;
      gb->gap_end -= move_size;
    }
  else if (position > gb->gap_start)
    {
      size_t move_size = position - gb->gap_start;
      memmove (gb->buffer + gb->gap_start, gb->buffer + gb->gap_end,
               move_size);
      gb->gap_start = position;
      gb->gap_end += move_size;
    }
}

int
gap_buffer_insert_char (GapBuffer *gb, char c)
{
  if (gap_buffer_gap_size (gb) == 0)
    {
      if (gap_buffer_ensure_capacity (gb, gb->capacity + MIN_GAP_SIZE) != 0)
        return -1;
    }

  gb->buffer[gb->gap_start] = c;
  gb->gap_start++;
  return 0;
}

int
gap_buffer_insert_string (GapBuffer *gb, const char *str)
{
  if (!str)
    return 0;

  size_t str_len = strlen (str);
  if (str_len == 0)
    return 0;

  if (gap_buffer_gap_size (gb) < str_len)
    {
      if (gap_buffer_ensure_capacity (gb, gb->capacity + str_len + MIN_GAP_SIZE)
          != 0)
        return -1;
    }

  /* Gap is now guaranteed to have room — copy in one shot. */
  memcpy (gb->buffer + gb->gap_start, str, str_len);
  gb->gap_start += str_len;
  return 0;
}

void
gap_buffer_delete_char (GapBuffer *gb)
{
  if (gb->gap_end < gb->capacity)
    {
      gb->gap_end++;
    }
}

void
gap_buffer_delete_char_before (GapBuffer *gb)
{
  if (gb->gap_start > 0)
    {
      gb->gap_start--;
    }
}

char
gap_buffer_get_char_at (const GapBuffer *gb, size_t position)
{
  if (position >= gap_buffer_length (gb))
    {
      return '\0';
    }

  if (position < gb->gap_start)
    {
      return gb->buffer[position];
    }
  else
    {
      return gb->buffer[position + gap_buffer_gap_size (gb)];
    }
}

char *
gap_buffer_to_string (const GapBuffer *gb)
{
  size_t length = gap_buffer_length (gb);
  char *result = malloc (length + 1);
  if (!result)
    return NULL;

  memcpy (result, gb->buffer, gb->gap_start);

  size_t after_gap_size = gb->capacity - gb->gap_end;
  memcpy (result + gb->gap_start, gb->buffer + gb->gap_end, after_gap_size);

  result[length] = '\0';
  return result;
}

void
gap_buffer_print_debug (const GapBuffer *gb)
{
  printf ("Gap Buffer Debug:\n");
  printf ("  Capacity: %zu\n", gb->capacity);
  printf ("  Gap: [%zu, %zu) size=%zu\n", gb->gap_start, gb->gap_end,
          gap_buffer_gap_size (gb));
  printf ("  Length: %zu\n", gap_buffer_length (gb));
  printf ("  Cursor: %zu\n", gap_buffer_cursor_position (gb));

  printf ("  Buffer: \"");
  for (size_t i = 0; i < gb->capacity; i++)
    {
      if (i >= gb->gap_start && i < gb->gap_end)
        {
          printf ("_");
        }
      else
        {
          char c = gb->buffer[i];
          if (c >= 32 && c <= 126)
            {
              printf ("%c", c);
            }
          else
            {
              printf ("\\x%02x", (unsigned char)c);
            }
        }
    }
  printf ("\"\n");
}
