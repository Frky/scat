/* Create an inode relative to an open directory.
   Copyright (C) 2009-2017 Free Software Foundation, Inc.

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

/* written by Eric Blake */

#include <config.h>

#include <sys/stat.h>

#if !HAVE_MKNOD

# include <errno.h>

/* Mingw lacks mknod, so this wrapper is trivial.  */

int
mknodat (int fd _GL_UNUSED, char const *path _GL_UNUSED,
         mode_t mode _GL_UNUSED, dev_t dev _GL_UNUSED)
{
  errno = ENOSYS;
  return -1;
}

#else /* HAVE_MKFIFO */

/* Create a file system node FILE relative to directory FD, with
   access permissions and file type in MODE, and device type in DEV.
   Usually, non-root applications can only create named fifos, with
   DEV set to 0.  If possible, create the node without changing the
   working directory.  Otherwise, resort to using save_cwd/fchdir,
   then mknod/restore_cwd.  If either the save_cwd or the restore_cwd
   fails, then give a diagnostic and exit nonzero.  */

# define AT_FUNC_NAME mknodat
# define AT_FUNC_F1 mknod
# define AT_FUNC_POST_FILE_PARAM_DECLS , mode_t mode, dev_t dev
# define AT_FUNC_POST_FILE_ARGS        , mode, dev
# include "at-func.c"
# undef AT_FUNC_NAME
# undef AT_FUNC_F1
# undef AT_FUNC_POST_FILE_PARAM_DECLS
# undef AT_FUNC_POST_FILE_ARGS

#endif /* HAVE_MKFIFO */
