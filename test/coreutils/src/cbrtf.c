/* Compute cubic root of float value.
   Copyright (C) 1997, 2012-2017 Free Software Foundation, Inc.

   Contributed by Dirk Alboth <dirka@uni-paderborn.de> and
   Ulrich Drepper <drepper@cygnus.com>, 1997.

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
#include <math.h>

/* MSVC with option -fp:strict refuses to compile constant initializers that
   contain floating-point operations.  Pacify this compiler.  */
#ifdef _MSC_VER
# pragma fenv_access (off)
#endif

/* Code based on glibc/sysdeps/ieee754/flt-32/s_cbrtf.c.  */

#define CBRT2 1.2599210498948731648             /* 2^(1/3) */
#define SQR_CBRT2 1.5874010519681994748         /* 2^(2/3) */

static const double factor[5] =
{
  1.0 / SQR_CBRT2,
  1.0 / CBRT2,
  1.0,
  CBRT2,
  SQR_CBRT2
};


float
cbrtf (float x)
{
  if (isfinite (x) && x != 0.0f)
    {
      float xm, ym, u, t2;
      int xe;

      /* Reduce X.  XM now is an range 1.0 to 0.5.  */
      xm = frexpf (fabsf (x), &xe);

      u = (0.492659620528969547
           + (0.697570460207922770 - 0.191502161678719066 * xm) * xm);

      t2 = u * u * u;

      ym = u * (t2 + 2.0 * xm) / (2.0 * t2 + xm) * factor[2 + xe % 3];

      return ldexpf (x > 0.0 ? ym : -ym, xe / 3);
    }
  else
    return x + x;
}
