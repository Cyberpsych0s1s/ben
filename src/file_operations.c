// SPDX-License-Identifier: MIT
#define _POSIX_C_SOURCE 200809L

#include "data_structures.h"
#include "gap_buffer.h"
#include "text_editor_functions.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

/*
 * Portable line reader. Returns:
 *   1 on success, *line points to a NUL-terminated line (trailing '\n' stripped
 *     if present). *had_newline is set to 1 if the line ended with '\n',
 *     0 if it was the final partial line (EOF reached mid-line).
 *   0 on clean EOF with nothing buffered.
 *  -1 on read error or OOM.
 *
 * Caller owns *line and *cap across calls; pass &NULL, &0 the first time and
 * free(*line) when done.
 */
static int
read_line_dynamic (FILE *file, char **line, size_t *cap, int *had_newline)
{
  size_t len = 0;
  *had_newline = 0;
  if (*line == NULL)
    {
      *cap = 128;
      *line = malloc (*cap);
      if (!*line)
        return -1;
    }

  for (;;)
    {
      if (len + 1 >= *cap)
        {
          size_t new_cap = *cap * 2;
          char *grown = realloc (*line, new_cap);
          if (!grown)
            return -1;
          *line = grown;
          *cap = new_cap;
        }

      if (fgets (*line + len, (int)(*cap - len), file) == NULL)
        {
          if (len > 0)
            {
              (*line)[len] = '\0';
              return 1;
            }
          return ferror (file) ? -1 : 0;
        }

      len += strlen (*line + len);

      if (len > 0 && (*line)[len - 1] == '\n')
        {
          (*line)[len - 1] = '\0';
          *had_newline = 1;
          return 1;
        }
      /* otherwise: buffer was filled without seeing '\n' — grow and continue */
    }
}

#ifdef _WIN32
#include <io.h>
#define ben_fsync _commit
#define ben_unlink _unlink
#else
#include <unistd.h>
#define ben_fsync fsync
#define ben_unlink unlink
#endif

void
insert_line_after (TextBuffer *buffer, Line *prev_line, Line *new_line)
{
  if (!buffer || !new_line)
    return;

  if (prev_line == NULL)
    {
      insert_line_at_beginning (buffer, new_line);
      return;
    }

  new_line->next = prev_line->next;
  prev_line->next = new_line;
  new_line->prev = prev_line;

  if (new_line->next != NULL)
    {
      new_line->next->prev = new_line;
    }

  buffer->num_lines++;

  if (prev_line == buffer->tail)
    {
      buffer->tail = new_line;
    }
}

void
insert_line_at_beginning (TextBuffer *buffer, Line *new_line)
{
  if (!buffer || !new_line)
    return;

  new_line->next = buffer->head;
  new_line->prev = NULL;

  if (buffer->head != NULL)
    {
      buffer->head->prev = new_line;
    }
  else
    {
      buffer->tail = new_line;
    }

  buffer->head = new_line;
  buffer->num_lines++;
}

void
init_editor_buffer (TextBuffer *buffer)
{
  if (!buffer)
    return;

  buffer->head = NULL;
  buffer->tail = NULL;
  buffer->num_lines = 0;
  buffer->current_line_node = NULL;
  buffer->current_col_offset = 0;
  buffer->has_final_newline = 1; /* POSIX default for new files */
}

Line *
create_new_line (const char *content)
{
  if (!content)
    content = "";

  Line *new_line = (Line *)malloc (sizeof (Line));
  if (new_line == NULL)
    return NULL;

  new_line->gb = gap_buffer_create (strlen (content) + 16);
  if (new_line->gb == NULL)
    {
      free (new_line);
      return NULL;
    }

  if (gap_buffer_insert_string (new_line->gb, content) != 0)
    {
      gap_buffer_destroy (new_line->gb);
      free (new_line);
      return NULL;
    }

  new_line->next = NULL;
  new_line->prev = NULL;
  return new_line;
}

Line *
create_new_line_empty ()
{
  Line *new_line = (Line *)malloc (sizeof (Line));
  if (new_line == NULL)
    return NULL;

  new_line->gb = gap_buffer_create (16);
  if (new_line->gb == NULL)
    {
      free (new_line);
      return NULL;
    }

  new_line->next = NULL;
  new_line->prev = NULL;
  return new_line;
}

void
insert_line_at_end (TextBuffer *buffer, Line *new_line)
{
  if (!buffer || !new_line)
    return;

  if (buffer->tail == NULL)
    {
      buffer->head = new_line;
      buffer->tail = new_line;
    }
  else
    {
      buffer->tail->next = new_line;
      new_line->prev = buffer->tail;
      buffer->tail = new_line;
    }
  buffer->num_lines++;
}

void
free_editor_buffer (TextBuffer *buffer)
{
  if (!buffer)
    return;

  Line *current = buffer->head;
  while (current != NULL)
    {
      Line *temp = current;
      current = current->next;
      gap_buffer_destroy (temp->gb);
      free (temp);
    }
  buffer->head = NULL;
  buffer->tail = NULL;
  buffer->num_lines = 0;
  buffer->current_line_node = NULL;
  buffer->current_col_offset = 0;
}

int
saveToFile (const char *filename, TextBuffer *buffer, int *lines_out,
            size_t *bytes_out)
{
  if (!filename || !buffer)
    {
      errno = EINVAL;
      return -1;
    }

  size_t tmppath_len = strlen (filename) + 5;
  char *tmppath = malloc (tmppath_len);
  if (!tmppath)
    return -1;
  snprintf (tmppath, tmppath_len, "%s.tmp", filename);

  FILE *file = fopen (tmppath, "w");
  if (!file)
    {
      int saved_errno = errno;
      free (tmppath);
      errno = saved_errno;
      return -1;
    }

  int line_count = 0;
  size_t byte_count = 0;

  for (Line *current = buffer->head; current != NULL; current = current->next)
    {
      char *line_text = line_to_string (current);
      if (!line_text)
        continue;

      /* The last line gets a trailing newline only if the original file had
         one. All other lines always do. */
      int is_last = (current->next == NULL);
      const char *terminator
          = (is_last && !buffer->has_final_newline) ? "" : "\n";
      int n = fprintf (file, "%s%s", line_text, terminator);
      free (line_text);

      if (n < 0)
        {
          int saved_errno = errno;
          fclose (file);
          ben_unlink (tmppath);
          free (tmppath);
          errno = saved_errno;
          return -1;
        }
      byte_count += (size_t)n;
      line_count++;
    }

  if (fflush (file) != 0)
    {
      int saved_errno = errno;
      fclose (file);
      ben_unlink (tmppath);
      free (tmppath);
      errno = saved_errno;
      return -1;
    }

  int fd = fileno (file);
  if (fd >= 0)
    ben_fsync (fd); /* best-effort */

  if (fclose (file) != 0)
    {
      int saved_errno = errno;
      ben_unlink (tmppath);
      free (tmppath);
      errno = saved_errno;
      return -1;
    }

  if (rename (tmppath, filename) != 0)
    {
      int saved_errno = errno;
      ben_unlink (tmppath);
      free (tmppath);
      errno = saved_errno;
      return -1;
    }

  free (tmppath);

  if (lines_out)
    *lines_out = line_count;
  if (bytes_out)
    *bytes_out = byte_count;
  return 0;
}

void
loadFromFile (const char *filename, TextBuffer *buffer)
{
  if (!filename || !buffer)
    return;

  FILE *file = fopen (filename, "r");
  if (file == NULL)
    {
      init_editor_buffer (buffer);
      Line *initial_line = create_new_line_empty ();
      insert_line_at_end (buffer, initial_line);
      buffer->current_line_node = initial_line;
      buffer->current_col_offset = 0;
      return;
    }

  free_editor_buffer (buffer);
  buffer->has_final_newline = 1; /* assume well-formed unless we learn otherwise */

  char *line_buffer = NULL;
  size_t cap = 0;

  int rc;
  int last_had_newline = 1;
  while ((rc = read_line_dynamic (file, &line_buffer, &cap, &last_had_newline))
         == 1)
    {
      Line *new_line = create_new_line (line_buffer);
      if (!new_line)
        break; /* OOM — return what we've read so far */
      insert_line_at_end (buffer, new_line);
    }

  buffer->has_final_newline = last_had_newline;

  free (line_buffer);
  fclose (file);

  if (buffer->head == NULL)
    {
      Line *initial_line = create_new_line_empty ();
      insert_line_at_end (buffer, initial_line);
      buffer->current_line_node = initial_line;
      buffer->current_col_offset = 0;
    }
  else
    {
      buffer->current_line_node = buffer->head;
      buffer->current_col_offset = 0;
    }
}

size_t
line_get_length (const Line *line)
{
  if (!line || !line->gb)
    return 0;
  return gap_buffer_length (line->gb);
}

char
line_get_char_at (const Line *line, size_t position)
{
  if (!line || !line->gb)
    return '\0';
  return gap_buffer_get_char_at (line->gb, position);
}

char *
line_to_string (const Line *line)
{
  if (!line || !line->gb)
    return NULL;
  return gap_buffer_to_string (line->gb);
}

int
line_insert_char_at (Line *line, size_t position, char c)
{
  if (!line || !line->gb)
    return -1;
  gap_buffer_move_cursor_to (line->gb, position);
  return gap_buffer_insert_char (line->gb, c);
}

int
line_insert_string_at (Line *line, size_t position, const char *str)
{
  if (!line || !line->gb || !str)
    return -1;
  gap_buffer_move_cursor_to (line->gb, position);
  return gap_buffer_insert_string (line->gb, str);
}

void
line_delete_char_at (Line *line, size_t position)
{
  if (!line || !line->gb)
    return;
  gap_buffer_move_cursor_to (line->gb, position);
  gap_buffer_delete_char (line->gb);
}

void
line_delete_char_before (Line *line, size_t position)
{
  if (!line || !line->gb || position == 0)
    return;
  gap_buffer_move_cursor_to (line->gb, position);
  gap_buffer_delete_char_before (line->gb);
}
