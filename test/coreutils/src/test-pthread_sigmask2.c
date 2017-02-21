/* Test of pthread_sigmask in a multi-threaded program.
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

/* Written by Bruno Haible <bruno@clisp.org>, 2011.  */

#include <config.h>

#include <signal.h>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include "glthread/thread.h"

#include "macros.h"

#if USE_POSIX_THREADS

static gl_thread_t main_thread;
static gl_thread_t killer_thread;

static void *
killer_thread_func (void *arg)
{
  sleep (1);
  pthread_kill (main_thread, SIGINT);
  return NULL;
}

static volatile int sigint_occurred;

static void
sigint_handler (int sig)
{
  sigint_occurred++;
}

int
main (int argc, char *argv[])
{
  sigset_t set;

  signal (SIGINT, sigint_handler);

  sigemptyset (&set);
  sigaddset (&set, SIGINT);

  /* Check error handling.  */
  ASSERT (pthread_sigmask (1729, &set, NULL) == EINVAL);

  /* Block SIGINT.  */
  ASSERT (pthread_sigmask (SIG_BLOCK, &set, NULL) == 0);

  /* Request a SIGINT signal from another thread.  */
  main_thread = gl_thread_self ();
  ASSERT (glthread_create (&killer_thread, killer_thread_func, NULL) == 0);

  /* Wait.  */
  sleep (2);

  /* The signal should not have arrived yet, because it is blocked.  */
  ASSERT (sigint_occurred == 0);

  /* Unblock SIGINT.  */
  ASSERT (pthread_sigmask (SIG_UNBLOCK, &set, NULL) == 0);

  /* The signal should have arrived now, because POSIX says
       "If there are any pending unblocked signals after the call to
        pthread_sigmask(), at least one of those signals shall be delivered
        before the call to pthread_sigmask() returns."  */
  ASSERT (sigint_occurred == 1);

  return 0;
}

#else

int
main ()
{
  fputs ("Skipping test: POSIX threads not enabled\n", stderr);
  return 77;
}

#endif
