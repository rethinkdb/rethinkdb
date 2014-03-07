// Copyright 2005-2006 Alexander Nasonov.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Status of some compilers:
//
// Microsoft (R) 32-bit C/C++ Optimizing Compiler Version 13.10.3077 for 80x86
// /Za (disable extentions) is totally broken.
// /Ze (enable extentions) promotes UIntEnum incorrectly to int.
// See http://lab.msdn.microsoft.com/ProductFeedback/viewfeedback.aspx?feedbackid=22b0a6b7-120f-4ca0-9136-fa1b25b26efe
//
// Intel 9.0.028 for Windows has a similar problem:
// https://premier.intel.com/IssueDetail.aspx?IssueID=365073
//
// gcc 3.4.4 with -fshort-enums option on x86
// Dummy value is required, otherwise gcc promotes Enum1
// to unsigned int although USHRT_MAX <= INT_MAX.
// http://gcc.gnu.org/bugzilla/show_bug.cgi?id=24063
// 
// CC: Sun WorkShop 6 update 2 C++ 5.3 Patch 111685-20 2004/03/19
// on SPARC V9 64-bit processor (-xarch=v9 flag)
// Dummy values are required for LongEnum3 and LongEnum4.
//
// CC: Sun C++ 5.7 Patch 117830-03 2005/07/21
// ICE in boost/type_traits/is_enum.hpp at line 67.


#include <climits>

#include "promote_util.hpp"
#include <boost/detail/workaround.hpp>

#ifdef BOOST_INTEL
//  remark #1418: external function definition with no prior declaration
#pragma warning(disable:1418)
#endif

enum IntEnum1 { IntEnum1_min = -5      , IntEnum1_max = 5        };
enum IntEnum2 { IntEnum2_min = SHRT_MIN, IntEnum2_max = SHRT_MAX };
enum IntEnum3 { IntEnum3_min = INT_MIN , IntEnum3_max = INT_MAX  };
enum IntEnum4 { IntEnum4_value = INT_MAX };
enum IntEnum5 { IntEnum5_value = INT_MIN };

void test_promote_to_int()
{
    test_cv<IntEnum1,int>();
    test_cv<IntEnum2,int>();
    test_cv<IntEnum3,int>();
    test_cv<IntEnum4,int>();
    test_cv<IntEnum5,int>();
}


#if !(defined(__GNUC__) && __GNUC__ == 3 && __GNUC_MINOR__ == 4 && USHRT_MAX <= INT_MAX)

enum Enum1 { Enum1_value = USHRT_MAX };

#else

// workaround for bug #24063 in gcc 3.4
// http://gcc.gnu.org/bugzilla/show_bug.cgi?id=24063
namespace gcc_short_enums_workaround {

enum short_enum { value = 1 };

template<bool ShortEnumsIsOn>
struct select
{
    // Adding negative dummy value doesn't change
    // promoted type because USHRT_MAX <= INT_MAX.
    enum type { dummy = -1, value = USHRT_MAX };
};

template<>
struct select<false>
{
    // No dummy value
    enum type { value = USHRT_MAX };
};

} // namespace gcc_short_enums_workaround

typedef gcc_short_enums_workaround::select<
    sizeof(gcc_short_enums_workaround::short_enum) != sizeof(int)
  >::type Enum1;

#endif

void test_promote_to_int_or_uint()
{
#if USHRT_MAX <= INT_MAX
    test_cv<Enum1, int>();
#else
    test_cv<Enum1, unsigned int>();
#endif
}

#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1500) ) || \
    BOOST_WORKAROUND(BOOST_INTEL_WIN, BOOST_TESTED_AT(1000))
// Don't test UIntEnum on VC++ 8.0 and Intel for Windows 9.0,
// they are broken. More info is on top of this file.
#else

enum UIntEnum { UIntEnum_max = UINT_MAX };

void test_promote_to_uint()
{
    test_cv< UIntEnum, unsigned int >();
}

#endif

// Enums can't be promoted to [unsigned] long if sizeof(int) == sizeof(long).
#if INT_MAX < LONG_MAX

enum LongEnum1 { LongEnum1_min = -1      , LongEnum1_max  = UINT_MAX };
enum LongEnum2 { LongEnum2_min = LONG_MIN, LongEnum2_max  = LONG_MAX };

enum LongEnum3
{
   LongEnum3_value = LONG_MAX
#if defined(__SUNPRO_CC) && __SUNPRO_CC <= 0x530
 , LongEnum3_dummy = -1
#endif
};

enum LongEnum4
{
    LongEnum4_value = LONG_MIN
#if defined(__SUNPRO_CC) && __SUNPRO_CC <= 0x530
  , LongEnum4_dummy = 1
#endif
};  

void test_promote_to_long()
{
    test_cv< LongEnum1, long >();
    test_cv< LongEnum2, long >();
    test_cv< LongEnum3, long >();
    test_cv< LongEnum4, long >();
}

enum ULongEnum { ULongEnum_value = ULONG_MAX };

void test_promote_to_ulong()
{
    test_cv< ULongEnum, unsigned long >();
}

#endif // #if INT_MAX < LONG_MAX

int main()
{
    return 0;
}

