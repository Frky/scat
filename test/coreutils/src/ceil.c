/* Round towards positive infinity.
   Copyright (C) 2007, 2010-2017 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2007.  */

#if ! defined USE_LONG_DOUBLE
# include <config.h>
#endif

/* Specification.  */
#include <math.h>

#include <float.h>

#undef MIN

#ifdef USE_LONG_DOUBLE
# define FUNC ceill
# define DOUBLE long double
# define MANT_DIG LDBL_MANT_DIG
# define MIN LDBL_MIN
# define L_(literal) literal##L
#elif ! defined USE_FLOAT
# define FUNC ceil
# define DOUBLE double
# define MANT_DIG DBL_MANT_DIG
# define MIN DBL_MIN
# define L_(literal) literal
#else /* defined USE_FLOAT */
# define FUNC ceilf
# define DOUBLE float
# define MANT_DIG FLT_MANT_DIG
# define MIN FLT_MIN
# define L_(literal) literal##f
#endif

/* -0.0.  See minus-zero.h.  */
#if defined __hpux || defined __sgi || defined __ICC
# define MINUS_ZERO (-MIN * MIN)
#else
# define MINUS_ZERO L_(-0.0)
#endif

/* MSVC with option -fp:strict refuses to compile constant initializers that
   contain floating-point operations.  Pacify this compiler.  */
#ifdef _MSC_VER
# pragma fenv_access (off)
#endif

/* 2^(MANT_DIG-1).  */
static const DOUBLE TWO_MANT_DIG =
  /* Assume MANT_DIG <= 5 * 31.
     Use the identity
       n = floor(n/5) + floor((n+1)/5) + ... + floor((n+4)/5).  */
  (DOUBLE) (1U << ((MANT_DIG - 1) / 5))
  * (DOUBLE) (1U << ((MANT_DIG - 1 + 1) / 5))
  * (DOUBLE) (1U << ((MANT_DIG - 1 + 2) / 5))
  * (DOUBLE) (1U << ((MANT_DIG - 1 + 3) / 5))
  * (DOUBLE) (1U << ((MANT_DIG - 1 + 4) / 5));

DOUBLE
FUNC (DOUBLE x)
{
  /* The use of 'volatile' guarantees that excess precision bits are dropped
     at each addition step and before the following comparison at the caller's
     site.  It is necessary on x86 systems where double-floats are not IEEE
     compliant by default, to avoid that the results become platform and compiler
     option dependent.  'volatile' is a portable alternative to gcc's
     -ffloat-store option.  */
  volatile DOUBLE y = x;
  volatile DOUBLE z = y;

  if (z > L_(0.0))
    {
      /* Avoid rounding errors for values near 2^k, where k >= MANT_DIG-1.  */
      if (z < TWO_MANT_DIG)
        {
          /* Round to the next integer (nearest or up or down, doesn't matter).  */
          z += TWO_MANT_DIG;
          z -= TWO_MANT_DIG;
          /* Enforce rounding up.  */
          if (z < y)
            z += L_(1.0);
        }
    }
  else if (z < L_(0.0))
    {
      /* For -1 < x < 0, return -0.0 regardless of the current rounding
         mode.  */
      if (z > L_(-1.0))
        z = MINUS_ZERO;
      /* Avoid rounding errors for values near -2^k, where k >= MANT_DIG-1.  */
      else if (z > - TWO_MANT_DIG)
        {
          /* Round to the next integer (nearest or up or down, doesn't matter).  */
          z -= TWO_MANT_DIG;
          z += TWO_MANT_DIG;
          /* Enforce rounding up.  */
          if (z < y)
            z += L_(1.0);
        }
    }
  return z;
}
