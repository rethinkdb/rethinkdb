///////////////////////////////////////////////////////////////////////////////
// test4.hpp
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

    // for testing recursive static regexes
    //regex_type parens = L('(') >> *( keep( +~(set=L('('),L(')')) ) | self ) >> L(')');

    regex_type parens;
    parens = L('(') >> *( keep( +~(set=L('('),L(')')) ) | by_ref(parens) ) >> L(')');

    static char_type const *nilbr = 0;
    static xpr_test_case const test_cases[] =
    {
        xpr_test_case
        (
            "test61"
          , L("this is sublist(now(is(the(time),for(all),good(men))to(come)))ok?")
          , regex_type(_b >> L("sublist") >> parens)
          , backrefs(L("sublist(now(is(the(time),for(all),good(men))to(come)))"), nilbr)
        )
      , xpr_test_case
        (
            "test62"
          , L("this is sublist(now(is(the(time),for(all),good(men))to(come))ok?")
          , regex_type(_b >> L("sublist") >> parens)
          , no_match
        )
      , xpr_test_case
        (
            "test63"
          , L("foobar")
          , regex_type(bos >> L("baz") | L("bar"))
          , backrefs(L("bar"), nilbr)
        )
      , xpr_test_case
        (
            "test69"
          , L("FooBarfoobar")
          , regex_type(icase(*_ >> L("foo")))
          , backrefs(L("FooBarfoo"), nilbr)
        )
      , xpr_test_case
        (
            "test70"
          , L("FooBarfoobar")
          , regex_type(icase(*_ >> L("boo")))
          , no_match
        )
      , xpr_test_case
        (
            "test71"
          , L("FooBarfoobar")
          , regex_type(icase(*_ >> L("boo") | L("bar")))
          , backrefs(L("Bar"), nilbr)
        )
      , xpr_test_case
        (
            "test72"
          , L("FooBarfoobar")
          , regex_type(icase(L("bar")))
          , backrefs(L("Bar"), nilbr)
        )
      , xpr_test_case
        (
            "test75"
          , L("fooooo")
          , regex_type(L('f') >> repeat<1,repeat_max>(L('o')))
          , backrefs(L("fooooo"), nilbr)
        )
      , xpr_test_case
        (
            "test78"
          , L("This (has) parens")
          , regex_type(L("This ") >> (s1= L("(has)")) >> L(' ') >> (s2= L("parens")))
          , backrefs(L("This (has) parens"), L("(has)"), L("parens"), nilbr)
        )
      , xpr_test_case
        (
            "test79"
          , L("This (has) parens")
          , regex_type(as_xpr(L("This (has) parens")))
          , backrefs(L("This (has) parens"), nilbr)
        )
      , xpr_test_case
        (
            "test80"
          , L("This (has) parens")
          , regex_type(as_xpr(L("This (has) parens")))
          , backrefs(L("This (has) parens"), nilbr)
        )
    };

    return boost::make_iterator_range(test_cases);
}
