///////////////////////////////////////////////////////////////////////////////
// test11.hpp
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
            "test166"
          , L("G")
          , regex_type(L('f') | icase(L('g')))
          , backrefs(L("G"), nilbr)
        )
      , xpr_test_case
        (
            "test167"
          , L("aBBa")
          , regex_type(icase(+lower))
          , backrefs(L("aBBa"), nilbr)
        )
      , xpr_test_case
        (
            "test168"
          , L("aA")
          , regex_type(icase(+as_xpr(L('\x61'))))
          , backrefs(L("aA"), nilbr)
        )
      , xpr_test_case
        (
            "test169"
          , L("aA")
          , regex_type(icase(+set[L('\x61')]))
          , backrefs(L("aA"), nilbr)
        )
      , xpr_test_case
        (
            "test170"
          , L("aA")
          , regex_type(icase(+as_xpr(L('\x0061'))))
          , backrefs(L("aA"), nilbr)
        )
      , xpr_test_case
        (
            "test171"
          , L("aA")
          , regex_type(icase(+set[L('\x0061')]))
          , backrefs(L("aA"), nilbr)
        )
      , xpr_test_case
        (
            "test172"
          , L("abcd")
          , regex_type(L('a') >> +(s1= L('b') | (s2= *(s3= L('c')))) >> L('d'))
          , backrefs(L("abcd"), L(""), L(""), L(""), nilbr)
        )
      , xpr_test_case
        (
            "test173"
          , L("abcd")
          , regex_type(L('a') >> +(s1= L('b') | (s2= !(s3= L('c')))) >> L('d'))
          , backrefs(L("abcd"), L(""), L(""), L(""), nilbr)
        )
    };

    return boost::make_iterator_range(test_cases);
}

