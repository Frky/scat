/* Test of cosf() function.
   Copyright (C) 2010-2017 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2010-2011.  */

#include <config.h>

#include <math.h>

#include "signature.h"
SIGNATURE_CHECK (cosf, float, (float));

#include "macros.h"

volatile float x;
float y;

int
main ()
{
  /* A particular value.  */
  x = 0.6f;
  y = cosf (x);
  ASSERT (y >= 0.8253355f && y <= 0.8253357f);

  return 0;
}
