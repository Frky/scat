/* Test of filtering of data through a subprocess.
   Copyright (C) 2009-2017 Free Software Foundation, Inc.
   Written by Bruno Haible <haible@clisp.cons.org>, 2009.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <config.h>

#include "pipe-filter.h"

#include "binary-io.h"
#include "c-ctype.h"
#include "read-file.h"
#include "macros.h"


/* Pipe a text file through 'LC_ALL=C tr "[a-z]" "[A-Z]"', or equivalently,
   'tr "abcdefghijklmnopqrstuvwxyz" "ABCDEFGHIJKLMNOPQRSTUVWXYZ"', which
   converts ASCII characters from lower case to upper case.  */

struct locals
{
  const char *input;
  size_t nread;
  char buf[19];
};

static void *
prepare_read (size_t *num_bytes_p, void *private_data)
{
  struct locals *l = (struct locals *) private_data;
  *num_bytes_p = sizeof (l->buf);
  return l->buf;
}

static void
done_read (void *data_read, size_t num_bytes_read, void *private_data)
{
  struct locals *l = (struct locals *) private_data;
  const char *p = l->input + l->nread;
  const char *q = (const char *) data_read;
  size_t i;

  for (i = 0; i < num_bytes_read; i++, q++)
    {
      /* Handle conversion NL -> CRLF possibly done by the child process.  */
      if (!(O_BINARY && *q == '\r'))
        {
          char orig = *p;
          char expected = c_toupper (orig);
          ASSERT (*q == expected);
          p++;
        }
    }
  l->nread = p - l->input;
}

int
main (int argc, char *argv[])
{
  const char *tr_program;
  const char *input_filename;
  size_t input_size;
  char *input;

  ASSERT (argc == 3);

  tr_program = argv[1];

  /* Read some text from a file.  */
  input_filename = argv[2];
  input = read_binary_file (input_filename, &input_size);
  ASSERT (input != NULL);

  /* Convert it to uppercase, line by line.  */
  {
    const char *argv[4];
    struct locals l;
    struct pipe_filter_gi *f;
    int result;

    l.input = input;
    l.nread = 0;

    argv[0] = tr_program;
    argv[1] = "abcdefghijklmnopqrstuvwxyz";
    argv[2] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    argv[3] = NULL;

    f = pipe_filter_gi_create ("tr", tr_program, argv, false, true,
                               prepare_read, done_read, &l);
    ASSERT (f != NULL);
    result = pipe_filter_gi_write (f, input, input_size);
    ASSERT (result == 0);
    result = pipe_filter_gi_close (f);
    ASSERT (result == 0);
    ASSERT (l.nread == input_size);
  }

  return 0;
}
