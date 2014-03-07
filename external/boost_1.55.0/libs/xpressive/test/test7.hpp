///////////////////////////////////////////////////////////////////////////////
// test7.hpp
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
            "test127"
          , L("foobar")
          , regex_type(as_xpr(L("foo")) >> /*This is a comment[*/ L("bar"))
          , backrefs(L("foobar"), nilbr)
        )
      , xpr_test_case
        (
            "test128"
          , L("foobar")
          , regex_type(bos >> L("foobar") >> eos)
          , backrefs(L("foobar"), nilbr)
        )
      , xpr_test_case
        (
            "test129"
          , L("foobar")
          , regex_type(bos >> L('f') >> *as_xpr(L('o')))
          , backrefs(L("foo"), nilbr)
        )
      , xpr_test_case
        (
            "test129.1"
          , L("foobar")
          , regex_type(bos >> L('f') >> *as_xpr(L('\157')))
          , backrefs(L("foo"), nilbr)
        )
      , xpr_test_case
        (
            "test130"
          , L("foo bar")
          , regex_type(bos >> L("foo bar") >> eos)
          , backrefs(L("foo bar"), nilbr)
        )
      , xpr_test_case
        (
            "test131"
          , L("foo bar")
          , regex_type(bos >> L("foo") >> set[L(' ')] >> L("bar") >> eos)
          , backrefs(L("foo bar"), nilbr)
        )
      , xpr_test_case
        (
            "test132"
          , L("foo bar")
          , regex_type(bos >> (L("foo") >> set[L(' ')] >> L("bar")) >> eos /*This is a comment*/)
          , backrefs(L("foo bar"), nilbr)
        )
      , xpr_test_case
        (
            "test133"
          , L("foo bar")
          , regex_type(bos >> L("foo") >> set[L(' ')] >> L("bar") /*This is a comment*/)
          , backrefs(L("foo bar"), nilbr)
        )
      , xpr_test_case
        (
            "test134"
          , L("foo bar#Thisisnotacomment")
          , regex_type(bos >> L("foo") >> set[L(' ')] >> L("bar#Thisisnotacomment"))
          , backrefs(L("foo bar#Thisisnotacomment"), nilbr)
        )
      , xpr_test_case
        (
            "test135"
          , L("f oo b ar")
          , regex_type(bos >> L("f oo b ar"))
          , backrefs(L("f oo b ar"), nilbr)
        )
      , xpr_test_case
        (
            "test137"
          , L("a--")
          , regex_type(bos >> *(s1= optional(L('a'))) >> eos)
          , no_match
        )
      , xpr_test_case
        (
            "test138"
          , L("a--")
          , regex_type(bos >> -*(s1= optional(L('a'))) >> eos)
          , no_match
        )
      , xpr_test_case
        (
            "test139"
          , L("bc")
          , regex_type(bos >> repeat<2>(s1= optional(L('b'))) >> L("bc") >> eos)
          , backrefs(L("bc"), L(""), nilbr)
        )
      , xpr_test_case
        (
            "test140"
          , L("bbc")
          , regex_type(bos >> repeat<2>(s1= optional(L('b'))) >> L("bc") >> eos)
          , backrefs(L("bbc"), L(""), nilbr)
        )
    };

    return boost::make_iterator_range(test_cases);
}
