///////////////////////////////////////////////////////////////////////////////
// test1.h
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
            "test1"
          , L("foobarboo")
          , regex_type(as_xpr(L("foo")))
          , backrefs(L("foo"), nilbr)
        )
      , xpr_test_case
        (
            "test2"
          , L("foobarboo")
          , regex_type(as_xpr(L("bar")))
          , backrefs(L("bar"), nilbr)
        )
      , xpr_test_case
        (
            "test3"
          , L("foobarboo")
          , regex_type(as_xpr(L("bat")))
          , no_match
        )
      , xpr_test_case
        (
            "test4"
          , L("foobarboo")
          , regex_type(L('b') >> *_ >> L("ar"))
          , backrefs(L("bar"), nilbr)
        )
      , xpr_test_case
        (
            "test5"
          , L("foobarboo")
          , regex_type(L('b') >> *_ >> L('r'))
          , backrefs(L("bar"), nilbr)
        )
      , xpr_test_case
        (
            "test6"
          , L("foobarboo")
          , regex_type(L('b') >> *_ >> L('b'))
          , backrefs(L("barb"), nilbr)
        )
      , xpr_test_case
        (
            "test7"
          , L("foobarboo")
          , regex_type(L('b') >> *_ >> L('o'))
          , backrefs(L("barboo"), nilbr)
        )
      , xpr_test_case
        (
            "test8"
          , L("foobarboo")
          , regex_type(L('b') >> *_ >> L("oo"))
          , backrefs(L("barboo"), nilbr)
        )
      , xpr_test_case
        (
            "test9"
          , L("foobarboo")
          , regex_type(L('b') >> +_ >> L("ar"))
          , no_match
        )
      , xpr_test_case
        (
            "test10"
          , L("foobarboo")
          , regex_type(L('b') >> +_ >> L('r'))
          , backrefs(L("bar"), nilbr)
        )
      , xpr_test_case
        (
            "test11"
          , L("foobarboo")
          , regex_type(L('b') >> +_ >> L('b'))
          , backrefs(L("barb"), nilbr)
        )
      , xpr_test_case
        (
            "test12"
          , L("foobarboo")
          , regex_type(L('b') >> +_ >> L('o'))
          , backrefs(L("barboo"), nilbr)
        )
      , xpr_test_case
        (
            "test13"
          , L("foobarboo")
          , regex_type(L('b') >> +_ >> L("oo"))
          , backrefs(L("barboo"), nilbr)
        )
      , xpr_test_case
        (
            "test14"
          , L("foobarboo")
          , regex_type(bos >> L("foo"))
          , backrefs(L("foo"), nilbr)
        )
      , xpr_test_case
        (
            "test15"
          , L("foobarboo")
          , regex_type(bos >> L('b') >> *_ >> L("ar"))
          , no_match
        )
      , xpr_test_case
        (
            "test15.1"
          , L("fOo")
          , regex_type(!icase(L('f') >> *as_xpr(L('o'))))
          , backrefs(L("fOo"), nilbr)
        )
    };

    return boost::make_iterator_range(test_cases);
}

