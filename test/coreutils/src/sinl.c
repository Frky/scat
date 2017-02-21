/* s_sinl.c -- long double version of s_sin.c.
 * Conversion to long double by Jakub Jelinek, jj@ultra.linux.cz.
 */

/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

#include <config.h>

/* Specification.  */
#include <math.h>

#if HAVE_SAME_LONG_DOUBLE_AS_DOUBLE

long double
sinl (long double x)
{
  return sin (x);
}

#else

/* Code based on glibc/sysdeps/ieee754/ldbl-128/s_sinl.c.  */

/* sinl(x)
 * Return sine function of x.
 *
 * kernel function:
 *      __kernel_sinl           ... sine function on [-pi/4,pi/4]
 *      __kernel_cosl           ... cosine function on [-pi/4,pi/4]
 *      __ieee754_rem_pio2l     ... argument reduction routine
 *
 * Method.
 *      Let S,C and T denote the sin, cos and tan respectively on
 *      [-PI/4, +PI/4]. Reduce the argument x to y1+y2 = x-k*pi/2
 *      in [-pi/4 , +pi/4], and let n = k mod 4.
 *      We have
 *
 *          n        sin(x)      cos(x)        tan(x)
 *     ----------------------------------------------------------
 *          0          S           C             T
 *          1          C          -S            -1/T
 *          2         -S          -C             T
 *          3         -C           S            -1/T
 *     ----------------------------------------------------------
 *
 * Special cases:
 *      Let trig be any of sin, cos, or tan.
 *      trig(+-INF)  is NaN, with signals;
 *      trig(NaN)    is that NaN;
 *
 * Accuracy:
 *      TRIG(x) returns trig(x) nearly rounded
 */

# include "trigl.h"

long double
sinl (long double x)
{
  long double y[2], z = 0.0L;
  int n;

  /* sinl(NaN) is NaN */
  if (isnanl (x))
    return x;

  /* |x| ~< pi/4 */
  if (x >= -0.7853981633974483096156608458198757210492
      && x <= 0.7853981633974483096156608458198757210492)
    return kernel_sinl (x, z, 0);

    /* sinl(Inf) is NaN, sinl(0) is 0 */
  else if (x + x == x)
    return x - x;               /* NaN */

  /* argument reduction needed */
  else
    {
      n = ieee754_rem_pio2l (x, y);
      switch (n & 3)
        {
        case 0:
          return kernel_sinl (y[0], y[1], 1);
        case 1:
          return kernel_cosl (y[0], y[1]);
        case 2:
          return -kernel_sinl (y[0], y[1], 1);
        default:
          return -kernel_cosl (y[0], y[1]);
        }
    }
}

#endif

#if 0
int
main (void)
{
  printf ("%.16Lg\n", sinl (0.7853981633974483096156608458198757210492));
  printf ("%.16Lg\n", sinl (0.7853981633974483096156608458198757210492 *29));
  printf ("%.16Lg\n", sinl (0.7853981633974483096156608458198757210492 *2));
  printf ("%.16Lg\n", sinl (0.7853981633974483096156608458198757210492 *30));
  printf ("%.16Lg\n", sinl (0.7853981633974483096156608458198757210492 *4));
  printf ("%.16Lg\n", sinl (0.7853981633974483096156608458198757210492 *32));
  printf ("%.16Lg\n", sinl (0.7853981633974483096156608458198757210492 *2/3));
  printf ("%.16Lg\n", sinl (0.7853981633974483096156608458198757210492 *4/3));
}
#endif
