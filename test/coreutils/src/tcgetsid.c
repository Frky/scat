/* Determine the session ID of the controlling terminal of the current process.
   Copyright (C) 2010-2017 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.  */

#include <config.h>

#define USE_OLD_TTY /* needed on OpenBSD 4.5, so that TIOCGSID gets defined */

/* Specification.  */
#include <termios.h>

#include <errno.h>
#include <sys/ioctl.h>

pid_t
tcgetsid (int fd)
{
#ifdef TIOCGSID /* Mac OS X, OpenBSD */
  int sid;

  if (ioctl (fd, TIOCGSID, &sid) < 0)
    return -1; /* errno is set here */

  return sid;
#else /* FreeBSD, Cygwin, mingw */
  errno = ENOSYS;
  return -1;
#endif
}
