///////////////////////////////////////////////////////////////////////////////
// test10.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "./test.hpp"

///////////////////////////////////////////////////////////////////////////////
// get_test_cases
//
template<typename BidiIterT>
boost::iterator_range<xpr_test_case<BidiIterT> const *> get_test_cases()
{
    typedef typename boost::iterator_value<BidiIterT>::type char_type;
    typedef xpr_test_case<BidiIterT> xpr_test_case;
    typedef basic_regex<BidiIterT> regex_type;

    static char_type const *nilbr = 0;
    static xpr_test_case const test_cases[] =
    {
        xpr_test_case
        (
            "test16"
          , L("foobarboo")
          , regex_type(L('b') >> +_ >> L("ar") >> eos)
          , no_match
        )
      , xpr_test_case
        (
            "test17"
          , L("foobarboo")
          , regex_type(L('b') >> +_ >> L('o') >> eos)
          , backrefs(L("barboo"), nilbr)
        )
      , xpr_test_case
        (
            "test18"
          , L("foobarboo")
          , regex_type(L('b') >> +_ >> L("oo") >> eos)
          , backrefs(L("barboo"), nilbr)
        )
      , xpr_test_case
        (
            "test19"
          , L("+1234.56789F")
          , regex_type(bos >> (s1= !(set=L('-'),L('+')) >> +range(L('0'),L('9'))
                           >> !(s2= L('.') >> *range(L('0'),L('9'))))
                           >> (s3= (set=L('C'),L('F'))) >> eos)
          , backrefs(L("+1234.56789F"), L("+1234.56789"), L(".56789"), L("F"), nilbr)
        )
      , xpr_test_case
        (
            "test20"
          , L("+1234.56789")
          , regex_type( !(s1= as_xpr(L('+'))|L('-')) >> (s2= +range(L('0'),L('9')) >> !as_xpr(L('.')) >> *range(L('0'),L('9')) |
                        L('.') >> +range(L('0'),L('9'))) >> !(s3= (set=L('e'),L('E')) >> !(s4= as_xpr(L('+'))|L('-')) >> +range(L('0'),L('9'))))
          , backrefs(L("+1234.56789"), L("+"), L("1234.56789"), L(""), L(""), nilbr)
        )
    };

    return boost::make_iterator_range(test_cases);
}

