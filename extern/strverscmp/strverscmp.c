/* Compare strings while treating digits characters numerically.
   Copyright (C) 1997, 2000, 2002, 2004 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Jean-François Bignolles <bignolle@ecoledoc.ibp.fr>, 1997.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* Modified to recognise negative numbers. Copyright (c) 2015 cc9cii */
/* Modified to recognise hex numbers. Copyright (c) 2016 cc9cii */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <ctype.h>

/* states: S_N: normal, S_I: comparing integral part, S_F: comparing
           fractional parts, S_Z: idem but with leading Zeroes only */
#define S_N    0x0
#define S_I    0x4
#define S_F    0x8
#define S_Z    0xc

/* result_type: CMP: return diff; LEN: compare using len_diff/diff */
#define CMP    2
#define LEN    3


/* ISDIGIT differs from isdigit, as follows:
   - Its arg may be any int or unsigned int; it need not be an unsigned char.
   - It's guaranteed to evaluate its argument exactly once.
   - It's typically faster.
   POSIX says that only '0' through '9' are digits.  Prefer ISDIGIT to
   ISDIGIT_LOCALE unless it's important to use the locale's definition
   of `digit' even when the host does not conform to POSIX.  */
#define ISDIGIT(c) ((unsigned int) (c) - '0' <= 9)

/* ASCII
   0 -> 48
   9 -> 57
   A -> 65, a -> 97  (A = a-32)
   F -> 70, f -> 102 (F = f-32)
   for now, just use isxdigit() */

#undef __strverscmp
#undef strverscmp

#ifndef weak_alias
# define __strverscmp strverscmp
#endif

/* Compare S1 and S2 as strings holding indices/version numbers,
   returning less than, equal to or greater than zero if S1 is less than,
   equal to or greater than S2 (for more info, see the texinfo doc).
*/

int
__strverscmp (const char *s1, const char *s2, int hex)
{
  const unsigned char *p1 = (const unsigned char *) s1;
  const unsigned char *p2 = (const unsigned char *) s2;
  unsigned char c1, c2;
  int state;
  int diff;
  int negative = 1;

  /* Symbol(s)    0       [1-9]   others  (padding)
     Transition   (10) 0  (01) d  (00) x  (11) -   */
  static const unsigned int next_state[] =
  {
      /* state    x    d    0    - */
      /* S_N */  S_N, S_I, S_Z, S_N,
      /* S_I */  S_N, S_I, S_I, S_I,
      /* S_F */  S_N, S_F, S_F, S_F,
      /* S_Z */  S_N, S_F, S_Z, S_Z
  };

  static const int result_type[] =
  {
      /* state   x/x  x/d  x/0  x/-  d/x  d/d  d/0  d/-
                 0/x  0/d  0/0  0/-  -/x  -/d  -/0  -/- */

      /* S_N */  CMP, CMP, CMP, CMP, CMP, LEN, CMP, CMP,
                 CMP, CMP, CMP, CMP, CMP, CMP, CMP, CMP,
      /* S_I */  CMP, -1,  -1,  CMP,  1,  LEN, LEN, CMP,
                  1,  LEN, LEN, CMP, CMP, CMP, CMP, CMP,
      /* S_F */  CMP, CMP, CMP, CMP, CMP, LEN, CMP, CMP,
                 CMP, CMP, CMP, CMP, CMP, CMP, CMP, CMP,
      /* S_Z */  CMP,  1,   1,  CMP, -1,  CMP, CMP, CMP,
                 -1,  CMP, CMP, CMP
  };

  if (p1 == p2)
    return 0;

  c1 = *p1++;
  c2 = *p2++;
  if (c1 == '-')
    negative = -1;
  /* Hint: '0' is a digit too.  */
  state = S_N | ((c1 == '0') + (hex ? (isxdigit(c1) != 0) : (ISDIGIT (c1) != 0)));

  while ((diff = c1 - c2) == 0 && c1 != '\0')
    {
      state = next_state[state];
      c1 = *p1++;
      c2 = *p2++;
      state |= (c1 == '0') + (hex ? (isxdigit(c1) != 0) : (ISDIGIT (c1) != 0));
      if (negative == -1 && (state & 0x3) == 0)
        negative = 1; /* non-digit turns negative off */
      if (c1 == '-')
        negative = -1;
    }

  state = result_type[state << 2 | ((c2 == '0') + (hex ? (isxdigit(c2) != 0) : (ISDIGIT (c2) != 0)))];

  switch (state)
    {
    case CMP:
      if (c1 == '-' && (hex ? (isxdigit(c2) != 0) : (ISDIGIT (c2) != 0)))
        return -1;
      else if (c2 == '-' && (hex ? (isxdigit(c1) != 0) : (ISDIGIT (c1) != 0)))
        return 1;

      return negative * diff;

    case LEN:
      while (hex ? isxdigit(*p1++) : ISDIGIT (*p1++))
        if (!(hex ? isxdigit(*p2++) : ISDIGIT (*p2++)))
          return negative; /* longer negatives numbers are smaller */

      return negative * ((hex ? isxdigit(*p2) : ISDIGIT (*p2)) ? -1 : diff);

    default:
      return state;
    }
}
#ifdef weak_alias
weak_alias (__strverscmp, strverscmp)
#endif
