///////////////////////////////////////////////////////////////////////////////
// test8.hpp
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
            "test141"
          , L("bbbc")
          , regex_type(bos >> repeat<2>(s1= optional(L('b'))) >> L("bc") >> eos)
          , backrefs(L("bbbc"), L("b"), nilbr)
        )
      , xpr_test_case
        (
            "test142"
          , L("bbbbc")
          , regex_type(bos >> repeat<2>(s1= optional(L('b'))) >> L("bc") >> eos)
          , no_match
        )
      , xpr_test_case
        (
            "test143"
          , L("bbbbc")
          , regex_type(bos >> *(s1= optional(L('b'))) >> L('d') >> eos)
          , no_match
        )
      , xpr_test_case
        (
            "test144"
          , L("bc")
          , regex_type(bos >> -repeat<2>(s1= optional(L('b'))) >> L("bc") >> eos)
          , backrefs(L("bc"), L(""), nilbr)
        )
      , xpr_test_case
        (
            "test145"
          , L("bbc")
          , regex_type(bos >> -repeat<2>(s1= optional(L('b'))) >> L("bc") >> eos)
          , backrefs(L("bbc"), L(""), nilbr)
        )
      , xpr_test_case
        (
            "test146"
          , L("bbbc")
          , regex_type(bos >> -repeat<2>(s1= optional(L('b'))) >> L("bc") >> eos)
          , backrefs(L("bbbc"), L("b"), nilbr)
        )
      , xpr_test_case
        (
            "test147"
          , L("bbbbc")
          , regex_type(bos >> -repeat<2>(s1= optional(L('b'))) >> L("bc") >> eos)
          , no_match
        )
      , xpr_test_case
        (
            "test148"
          , L("bbbbc")
          , regex_type(bos >> -*(s1= optional(L('b'))) >> L('d') >> eos)
          , no_match
        )
      , xpr_test_case
        (
            "test149"
          , L("bc")
          , regex_type(bos >> repeat<2>(s1= -optional(L('b'))) >> L("bc") >> eos)
          , backrefs(L("bc"), L(""), nilbr)
        )
      , xpr_test_case
        (
            "test150"
          , L("bbc")
          , regex_type(bos >> repeat<2>(s1= -optional(L('b'))) >> L("bc") >> eos)
          , backrefs(L("bbc"), L("b"), nilbr)
        )
    };

    return boost::make_iterator_range(test_cases);
}
