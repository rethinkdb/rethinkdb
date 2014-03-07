/*
 *
 * Copyright (c) 2010
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#include <unicode/uversion.h>
#include <unicode/locid.h>
#include <unicode/utypes.h>
#include <unicode/uchar.h>
#include <unicode/coll.h>

#if defined(_MSC_VER) && !defined(_DLL)
#error "Mixing ICU with a static runtime doesn't work"
#endif

int main()
{
   icu::Locale loc;
   UErrorCode err = U_ZERO_ERROR;
   UChar32 c = ::u_charFromName(U_UNICODE_CHAR_NAME, "GREEK SMALL LETTER ALPHA", &err);
   return err;
}

