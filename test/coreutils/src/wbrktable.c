/* Word break auxiliary table.  -*- coding: utf-8 -*-
   Copyright (C) 2009-2017 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2009.

   This program is free software: you can redistribute it and/or modify it
   under the terms of the GNU Lesser General Public License as published
   by the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <config.h>

/* Specification.  */
#include "wbrktable.h"

/* This table contains the following rules (see UAX #29):

                           last         current

                     (ALetter | HL) × (ALetter | HL)                  (WB5)
                     (ALetter | HL) × Numeric                         (WB9)
                                 HL × SQ                              (WB7a)
                            Numeric × (ALetter | HL)                  (WB10)
                            Numeric × Numeric                         (WB8)
                           Katakana × Katakana                        (WB13)
(ALetter | HL | Numeric | Katakana) × ExtendNumLet                    (WB13a)
                       ExtendNumLet × ExtendNumLet                    (WB13a)
                   ExtendNumLet × (ALetter | HL | Numeric | Katakana) (WB13b)
                 Regional_Indicator × Regional_Indicator              (WB13c)
 */

const unsigned char uniwbrk_table[12][12] =
{        /* current:      OTHER        MIDNUMLET    NUMERIC     DQ         */
         /*                 KATAKANA     MIDLETTER    EXTENDNUMLET  SQ     */
         /*                   ALETTER      MIDNUM           RI          HL */
  /* last */
  /* WBP_OTHER */        {  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
  /* WBP_KATAKANA */     {  1,  0,  1,  1,  1,  1,  1,  0,  1,  1,  1,  1 },
  /* WBP_ALETTER */      {  1,  1,  0,  1,  1,  1,  0,  0,  1,  1,  1,  0 },
  /* WBP_MIDNUMLET */    {  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
  /* WBP_MIDLETTER */    {  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
  /* WBP_MIDNUM */       {  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
  /* WBP_NUMERIC */      {  1,  1,  0,  1,  1,  1,  0,  0,  1,  1,  1,  0 },
  /* WBP_EXTENDNUMLET */ {  1,  0,  0,  1,  1,  1,  0,  0,  1,  1,  1,  0 },
  /* WBP_RI */           {  1,  1,  1,  1,  1,  1,  1,  1,  0,  1,  1,  1 },
  /* WBP_DQ */           {  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
  /* WBP_SQ */           {  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
  /* WBP_HL */           {  1,  1,  0,  1,  1,  1,  0,  0,  1,  1,  0,  0 }
};
