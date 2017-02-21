/* Non-blocking I/O for pipe or socket descriptors.
   Copyright (C) 2011-2017 Free Software Foundation, Inc.

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

/* Specification.  */
#include "nonblocking.h"

#include <errno.h>

#if (defined _WIN32 || defined __WIN32__) && ! defined __CYGWIN__
/* Native Windows API.  */

# include <sys/ioctl.h>
# include <sys/socket.h>
# include <unistd.h>

/* Get declarations of the native Windows API functions.  */
# define WIN32_LEAN_AND_MEAN
# include <windows.h>

# include "msvc-nothrow.h"

int
get_nonblocking_flag (int desc)
{
  HANDLE h = (HANDLE) _get_osfhandle (desc);
  if (h == INVALID_HANDLE_VALUE)
    {
      errno = EBADF;
      return -1;
    }
  if (GetFileType (h) == FILE_TYPE_PIPE)
    {
      /* h is a pipe or socket.  */
      DWORD state;
      if (GetNamedPipeHandleState (h, &state, NULL, NULL, NULL, NULL, 0))
        /* h is a pipe.  */
        return (state & PIPE_NOWAIT) != 0;
      else
        /* h is a socket.  */
        errno = ENOSYS;
        return -1;
    }
  else
    /* The native Windows API does not support non-blocking on regular
       files.  */
    return 0;
}

int
set_nonblocking_flag (int desc, bool value)
{
  HANDLE h = (HANDLE) _get_osfhandle (desc);
  if (h == INVALID_HANDLE_VALUE)
    {
      errno = EBADF;
      return -1;
    }
  if (GetFileType (h) == FILE_TYPE_PIPE)
    {
      /* h is a pipe or socket.  */
      DWORD state;
      if (GetNamedPipeHandleState (h, &state, NULL, NULL, NULL, NULL, 0))
        {
          /* h is a pipe.  */
          if ((state & PIPE_NOWAIT) != 0)
            {
              if (value)
                return 0;
              state &= ~PIPE_NOWAIT;
            }
          else
            {
              if (!value)
                return 0;
              state |= PIPE_NOWAIT;
            }
          if (SetNamedPipeHandleState (h, &state, NULL, NULL))
            return 0;
          errno = EINVAL;
          return -1;
        }
      else
        {
          /* h is a socket.  */
          int v = value;
          return ioctl (desc, FIONBIO, &v);
        }
    }
  else
    {
      /* The native Windows API does not support non-blocking on regular
         files.  */
      if (!value)
        return 0;
      errno = ENOTSUP;
      return -1;
    }
}

#else
/* Unix API.  */

# include <fcntl.h>

# if GNULIB_defined_O_NONBLOCK
#  error Please port nonblocking to your platform
# endif

/* We don't need the gnulib replacement of fcntl() here.  */
# undef fcntl

int
get_nonblocking_flag (int desc)
{
  int fcntl_flags;

  fcntl_flags = fcntl (desc, F_GETFL, 0);
  if (fcntl_flags < 0)
    return -1;
  return (fcntl_flags & O_NONBLOCK) != 0;
}

int
set_nonblocking_flag (int desc, bool value)
{
  int fcntl_flags;

  fcntl_flags = fcntl (desc, F_GETFL, 0);
  if (fcntl_flags < 0)
    return -1;
  if (((fcntl_flags & O_NONBLOCK) != 0) == value)
    return 0;
  if (value)
    fcntl_flags |= O_NONBLOCK;
  else
    fcntl_flags &= ~O_NONBLOCK;
  return fcntl (desc, F_SETFL, fcntl_flags);
}

#endif
