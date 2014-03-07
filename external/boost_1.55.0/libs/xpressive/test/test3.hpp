///////////////////////////////////////////////////////////////////////////////
// test3.hpp
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
            "test41"
          , L("foobarFOObarfoo")
          , regex_type((s1= icase(L("FOO"))) >> (s2= -*_) >> s1)
          , backrefs(L("foobarFOObarfoo"), L("foo"), L("barFOObar"), nilbr)
        )
      , xpr_test_case
        (
            "test42"
          , L("foobarFOObarfoo")
          , regex_type((s1= icase(L("FOO"))) >> (s2= -*_) >> icase(s1))
          , backrefs(L("foobarFOO"), L("foo"), L("bar"), nilbr)
        )
      , xpr_test_case
        (
            "test42.1"
          , L("fooFOOOFOOOOObar")
          , regex_type(+(s1= L("foo") | icase(s1 >> L('O'))))
          , backrefs(L("fooFOOOFOOOO"), L("FOOOO"), nilbr)
        )
      , xpr_test_case
        (
            "test43"
          , L("zoo")
          , regex_type(bos >> set[range(L('A'),L('Z')) | range(L('a'),L('m'))])
          , no_match
        )
      , xpr_test_case
        (
            "test44"
          , L("Here is a URL: http://www.cnn.com. OK?")
          , regex_type((s1= L("http") >> !as_xpr(L('s')) >> L(":/") | L("www."))
                        >> +set[_w | (set=L('.'),L('/'),L(','),L('?'),L('@'),L('#'),L('%'),L('!'),L('_'),L('='),L('~'),L('&'),L('-'))]
                        >> _w)
          , backrefs(L("http://www.cnn.com"), L("http:/"), nilbr)
        )
      , xpr_test_case
        (
            "test45"
          , L("fooooooooo")
          , regex_type(L('f') >> repeat<2,5>(L('o')))
          , backrefs(L("fooooo"), nilbr)
        )
      , xpr_test_case
        (
            "test46"
          , L("fooooooooo")
          , regex_type(L('f') >> -repeat<2,5>(L('o')))
          , backrefs(L("foo"), nilbr)
        )
      , xpr_test_case
        (
            "test45.1"
          , L("fooooooooo")
          , regex_type(L('f') >> repeat<2,5>(L('o')) >> L('o'))
          , backrefs(L("foooooo"), nilbr)
        )
      , xpr_test_case
        (
            "test46.1"
          , L("fooooooooo")
          , regex_type(L('f') >> -repeat<2,5>(L('o')) >> L('o'))
          , backrefs(L("fooo"), nilbr)
        )
      , xpr_test_case
        (
            "test47"
          , L("{match this}")
          , regex_type(bos >> L('{') >> *_ >> L('}') >> eos)
          , backrefs(L("{match this}"), nilbr)
        )
      , xpr_test_case
        (
            "test48"
          , L("+-+-")
          , regex_type(+(set=L('+'),L('-')))
          , backrefs(L("+-+-"), nilbr)
        )
      , xpr_test_case
        (
            "test49"
          , L("+-+-")
          , regex_type(+(set=L('-'),L('+')))
          , backrefs(L("+-+-"), nilbr)
        )
      , xpr_test_case
        (
            "test50"
          , L("\\05g-9e")
          , regex_type(+set[_d | L('-') | L('g')])
          , backrefs(L("05g-9"), nilbr)
        )
      , xpr_test_case
        (
            "test51"
          , L("\\05g-9e")
          , regex_type(+set[_d | L('-') | L('g')])
          , backrefs(L("05g-9"), nilbr)
        )
      , xpr_test_case
        (
            "test52"
          , L("\\05g-9e")
          , regex_type(+set[L('g') | as_xpr(L('-')) | _d])
          , backrefs(L("05g-9"), nilbr)
        )
      , xpr_test_case
        (
            "test53"
          , L("\\05g-9e")
          , regex_type(+set[L('g') | as_xpr(L('-')) | _d])
          , backrefs(L("05g-9"), nilbr)
        )
      , xpr_test_case
        (
            "test54"
          , L("aBcdefg\\")
          , regex_type(icase(+range(L('a'),L('g'))))
          , backrefs(L("aBcdefg"), nilbr)
        )
      , xpr_test_case
        (
            "test55"
          , L("ab/.-ba")
          , regex_type(+range(L('-'),L('/')))
          , backrefs(L("/.-"), nilbr)
        )
      , xpr_test_case
        (
            "test56"
          , L("ab+,-ba")
          , regex_type(+range(L('+'),L('-')))
          , backrefs(L("+,-"), nilbr)
        )
      , xpr_test_case
        (
            "test56.1"
          , L("aaabbbb----")
          , regex_type(+range(L('b'),L('b')))
          , backrefs(L("bbbb"), nilbr)
        )
      , xpr_test_case
        (
            "test57"
          , L("foobarFOO5")
          , regex_type(icase((s1= L("foo")) >> *_ >> L('\15')))
          , no_match
        )
      , xpr_test_case
        (
            "test58"
          , L("Her number is 804-867-5309.")
          , regex_type(repeat<2>(repeat<3>(_d) >> L('-')) >> repeat<4>(_d))
          , backrefs(L("804-867-5309"), nilbr)
        )
      , xpr_test_case
        (
            "test59"
          , L("foo")
          , regex_type(L('f') >> +as_xpr(L('o')))
          , backrefs(L("foo"), nilbr)
        )
      , xpr_test_case
        (
            "test60"
          , L("fooFOObar")
          , regex_type(icase(+(s1= L("foo")) >> L("foobar")))
          , backrefs(L("fooFOObar"), L("foo"), nilbr)
        )
    };

    return boost::make_iterator_range(test_cases);
}
