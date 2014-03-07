///////////////////////////////////////////////////////////////////////////////
// test6.hpp
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
            "test103"
          , L("a\nxb\n")
          , regex_type(~before(bol) >> L('x'))
          , no_match
        )
      , xpr_test_case
        (
            "test104"
          , L("a\nxb\n")
          , regex_type(~before(bos) >> L('x'))
          , backrefs(L("x"), nilbr)
        )
      , xpr_test_case
        (
            "test105"
          , L("a\nxb\n")
          , regex_type(~before(bos) >> L('x'))
          , backrefs(L("x"), nilbr)
        )
      , xpr_test_case
        (
            "test106"
          , L("(this)")
          , regex_type(bos >> (L('(') >> (s1= nil) | (s2= nil)) >> +_w >> (L(')') >> s1 | s2) >> eos)
          , backrefs(L("(this)"), L(""), L(""), nilbr)
        )
      , xpr_test_case
        (
            "test107"
          , L("this")
          , regex_type(bos >> (L('(') >> (s1= nil) | (s2= nil)) >> +_w >> (L(')') >> s1 | s2) >> eos)
          , backrefs(L("this"), L(""), L(""), nilbr)
        )
      , xpr_test_case
        (
            "test108"
          , L("this)")
          , regex_type(bos >> (L('(') >> (s1= nil) | (s2= nil)) >> +_w >> (L(')') >> s1 | s2) >> eos)
          , no_match
        )
      , xpr_test_case
        (
            "test109"
          , L("(this")
          , regex_type(bos >> (L('(') >> (s1= nil) | (s2= nil)) >> +_w >> (L(')') >> s1 | s2) >> eos)
          , no_match
        )
      , xpr_test_case
        (
            "test110"
          , L("abba123abba")
          , regex_type(+~alpha)
          , backrefs(L("123"), nilbr)
        )
      , xpr_test_case
        (
            "test111"
          , L("abba123abba")
          , regex_type(+set[alpha | ~alpha])
          , backrefs(L("abba123abba"), nilbr)
        )
      , xpr_test_case
        (
            "test112"
          , L("123abba123")
          , regex_type(+~set[~alpha])
          , backrefs(L("abba"), nilbr)
        )
      //, xpr_test_case
      //  (
      //      "test113"
      //    , L("123abba123")
      //    , regex_type(as_xpr(L("[[:alpha:]\\y]+")))
      //    , backrefs(L("123abba123"), nilbr)
      //  )
      , xpr_test_case
        (
            "test114"
          , L("abba123abba")
          , regex_type(+~set[~alnum | ~digit])
          , backrefs(L("123"), nilbr)
        )
      , xpr_test_case
        (
            "test115"
          , L("aaaaA")
          , regex_type(icase(bos >> repeat<4>(s1= L('a') >> !s1) >> eos))
          , backrefs(L("aaaaA"), L("A"), nilbr)
        )
      , xpr_test_case
        (
            "test116"
          , L("aaaaAa")
          , regex_type(icase(bos >> repeat<4>(s1= L('a') >> !s1) >> eos))
          , backrefs(L("aaaaAa"), L("Aa"), nilbr)
        )
    };

    return boost::make_iterator_range(test_cases);
}
