/* Unicode Normalization Form KC.
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
#include "uninorm.h"

#include "normalize-internal.h"
#include "decompose-internal.h"

const struct unicode_normalization_form uninorm_nfkc =
  {
    NF_IS_COMPAT_DECOMPOSING | NF_IS_COMPOSING,
    uc_compat_decomposition,
    uc_composition,
    &uninorm_nfkd
  };
