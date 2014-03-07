///////////////////////////////////////////////////////////////////////////////
// test2.hpp
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

    // "(\\+|-)?([0-9]+\\.?[0-9]*|\\.[0-9]+)([eE](\\+|-)?[0-9]+)?"
    static regex_type const test21 = !(s1= as_xpr(L('+'))|L('-')) >> (s2= +range(L('0'),L('9')) >> !as_xpr(L('.')) >> *range(L('0'),L('9')) |
        L('.') >> +range(L('0'),L('9'))) >> !(s3= (set=L('e'),L('E')) >> !(s4= as_xpr(L('+'))|L('-')) >> +range(L('0'),L('9')));

    // "^(\\+|-)?([0-9]+\\.?[0-9]*|\\.[0-9]+)([eE](\\+|-)?[0-9]+)?$"
    static regex_type const test23 = bos >> test21 >> eos;

    static char_type const *nilbr = 0;
    static xpr_test_case const test_cases[] =
    {
        xpr_test_case
        (
            "test21"
          , L("-1234.56789")
          , test21
          , backrefs(L("-1234.56789"), L("-"), L("1234.56789"), L(""), L(""), nilbr)
        )
      , xpr_test_case
        (
            "test22"
          , L("-1234.56789e-123123123")
          , test21
          , backrefs(L("-1234.56789e-123123123"), L("-"), L("1234.56789"), L("e-123123123"), L("-"), nilbr)
        )
      , xpr_test_case
        (
            "test23"
          , L("-.1234.56789")
          , test23
          , no_match
        )
      , xpr_test_case
        (
            "test24"
          , L("-1234.56789e-12312e123")
          , test23
          , no_match
        )
      , xpr_test_case
        (
            "test25"
          , L("aaaaaaaaac")
          , regex_type(*as_xpr(L('a')) >> *as_xpr(L('a')) >> *as_xpr(L('a')) >> *as_xpr(L('a')) >> *as_xpr(L('a')) >> L('b'))
          , no_match
        )
      , xpr_test_case
        (
            "test26"
          , L("ThIs iS a TeSt")
          , regex_type(*set[range(L('a'),L('z')) | range(L('A'),L('Z'))])
          , backrefs(L("ThIs"), nilbr)
        )
      , xpr_test_case
        (
            "test27"
          , L("a^b")
          , regex_type(L('a') >> bos >> L('b'))
          , no_match
        )
      , xpr_test_case
        (
            "test28"
          , L("a^b")
          , regex_type(as_xpr(L("a^b")))
          , backrefs(L("a^b"), nilbr)
        )
      , xpr_test_case
        (
            "test29"
          , L("a^b")
          , regex_type(L('a') >> ~set[L(' ')] >> L('b'))
          , backrefs(L("a^b"), nilbr)
        )
      , xpr_test_case
        (
            "test30"
          , L("a^b")
          , regex_type(L('a') >> ~set[L('^')] >> L('b'))
          , no_match
        )
      , xpr_test_case
        (
            "test31"
          , L("a^b")
          , regex_type(L('a') >> ~set[L('^')] >> L('b'))
          , no_match
        )
      , xpr_test_case
        (
            "test31.1"
          , L("a^b")
          , regex_type(L('a') >> set[L('^')] >> L('b'))
          , backrefs(L("a^b"), nilbr)
        )
      , xpr_test_case
        (
            "test32"
          , L("foobazFOObar")
          , regex_type(icase(L("foo") >> before(L("bar"))))
          , backrefs(L("FOO"), nilbr)
        )
      , xpr_test_case
        (
            "test33"
          , L("foobarFOObaz")
          , regex_type(icase(L("foo") >> ~before(L("bar"))))
          , backrefs(L("FOO"), nilbr)
        )
      , xpr_test_case
        (
            "test34"
          , L("foobarFOO")
          , regex_type(icase(L("foo") >> ~before(L("bar"))))
          , backrefs(L("FOO"), nilbr)
        )
      , xpr_test_case
        (
            "test35"
          , L("foobarFOObarfoo")
          , regex_type(icase(+(s1= keep(s2= L("foo")) >> L("bar"))))
          , backrefs(L("foobarFOObar"), L("FOObar"), L("FOO"), nilbr)
        )
      , xpr_test_case
        (
            "test37"
          , L("foobar")
          , regex_type(+(s1= L("bar") | (s2= L("foo"))))
          , backrefs(L("foobar"), L("bar"), L("foo"), nilbr)
        )
      , xpr_test_case
        (
            "test38"
          , L("foobar")
          , regex_type(+(s1= (s2= L("bar")) | L("foo")))
          , backrefs(L("foobar"), L("bar"), L("bar"), nilbr)
        )
      , xpr_test_case
        (
            "test39"
          , L("foobar")
          , regex_type(+(s1= L("foo") | (s2= L("bar"))))
          , backrefs(L("foobar"), L("bar"), L("bar"), nilbr)
        )
      , xpr_test_case
        (
            "test40"
          , L("foobar")
          , regex_type(+(s1= (s2= L("foo")) | L("bar")))
          , backrefs(L("foobar"), L("bar"), L("foo"), nilbr)
        )
    };

    return boost::make_iterator_range(test_cases);
}
