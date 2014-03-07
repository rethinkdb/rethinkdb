///////////////////////////////////////////////////////////////////////////////
// test5.hpp
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
            "test82"
          , L("abba1234abba")
          , regex_type(+_d)
          , backrefs(L("1234"), nilbr)
        )
      , xpr_test_case
        (
            "test83"
          , L("1234abba1234")
          , regex_type(+~_d)
          , backrefs(L("abba"), nilbr)
        )
      , xpr_test_case
        (
            "test84"
          , L("abba1234abba")
          , regex_type(+set[_d])
          , backrefs(L("1234"), nilbr)
        )
      , xpr_test_case
        (
            "test85"
          , L("1234abba1234")
          , regex_type(+set[~_d])
          , backrefs(L("abba"), nilbr)
        )
      , xpr_test_case
        (
            "test86"
          , L("abba1234abba")
          , regex_type(+~set[~_d])
          , backrefs(L("1234"), nilbr)
        )
      , xpr_test_case
        (
            "test87"
          , L("1234abba1234")
          , regex_type(+~set[_d])
          , backrefs(L("abba"), nilbr)
        )
      , xpr_test_case
        (
            "test88"
          , L("1234abba1234")
          , regex_type(+set[~_w | ~_d])
          , backrefs(L("abba"), nilbr)
        )
      , xpr_test_case
        (
            "test89"
          , L("1234(.;)abba")
          , regex_type(+~set[_w | _d])
          , backrefs(L("(.;)"), nilbr)
        )
      , xpr_test_case
        (
            "test90"
          , L("(boo[bar]baz)")
          , regex_type((s1= L('(') >> (s2= nil) | L('[') >> (s3= nil)) >> -*_ >> (s4= L(')') >> s2 | L(']') >> s3))
          , backrefs(L("(boo[bar]baz)"), L("("), L(""), L(""), L(")"), nilbr)
        )
      , xpr_test_case
        (
            "test91"
          , L("[boo(bar)baz]")
          , regex_type((s1= L('(') >> (s2= nil) | L('[') >> (s3= nil)) >> -*_ >> (s4= L(')') >> s2 | L(']') >> s3))
          , backrefs(L("[boo(bar)baz]"), L("["), L(""), L(""), L("]"), nilbr)
        )
      , xpr_test_case
        (
            "test91"
          , L("[boo[bar]baz]")
          , regex_type((s1= L('(') >> (s2= nil) | L('[') >> (s3= nil)) >> -*_ >> (s4= L(')') >> s2 | L(']') >> s3))
          , backrefs(L("[boo[bar]"), L("["), L(""), L(""), L("]"), nilbr)
        )
      , xpr_test_case
        (
            "test92"
          , L("foobarfoo")
          , regex_type(after(L("foo")) >> L("bar"))
          , backrefs(L("bar"), nilbr)
        )
      , xpr_test_case
        (
            "test93"
          , L("foobarfoo")
          , regex_type(after(s1= L('f') >> _ >> L('o')) >> L("bar"))
          , backrefs(L("bar"), L("foo"), nilbr)
        )
      , xpr_test_case
        (
            "test94"
          , L("foOoo")
          , regex_type(icase(after(s1= L("fo")) >> L('o')))
          , backrefs(L("O"), L("fo"), nilbr)
        )
      , xpr_test_case
        (
            "test95"
          , L("fOooo")
          , regex_type(icase(~after(s1= L("fo")) >> L('o')))
          , backrefs(L("O"), L(""), nilbr)
        )
      , xpr_test_case
        (
            "test96"
          , L("12foo12")
          , regex_type(+alpha)
          , backrefs(L("foo"), nilbr)
        )
      , xpr_test_case
        (
            "test97"
          , L(";12foo12;")
          , regex_type(+set[alpha | digit])
          , backrefs(L("12foo12"), nilbr)
        )
      , xpr_test_case
        (
            "test98"
          , L("aaaa")
          , regex_type(after(s1= nil) >> L('a'))
          , backrefs(L("a"), L(""), nilbr)
        )
      , xpr_test_case
        (
            "test99"
          , L("ABCabc123foo")
          , regex_type(after(s1= L("abc") >> repeat<3>(_d)) >> L("foo"))
          , backrefs(L("foo"), L("abc123"), nilbr)
        )
    };

    return boost::make_iterator_range(test_cases);
}
