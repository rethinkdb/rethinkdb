
// Copyright Eric Niebler 2009
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: string.cpp 49240 2009-04-01 09:21:07Z eric_niebler $
// $Date: 2009-04-01 02:21:07 -0700 (Wed, 1 Apr 2009) $
// $Revision: 49240 $

#include <string>
#include <cstring>
#include <iostream>

#include <boost/mpl/string.hpp>

#include <boost/mpl/at.hpp>
#include <boost/mpl/back.hpp>
#include <boost/mpl/empty.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/erase.hpp>
#include <boost/mpl/insert.hpp>
#include <boost/mpl/advance.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector_c.hpp>
#include <boost/mpl/pop_back.hpp>
#include <boost/mpl/pop_front.hpp>
#include <boost/mpl/push_back.hpp>
#include <boost/mpl/push_front.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/detail/lightweight_test.hpp>

namespace mpl = boost::mpl;

// Accept a string as a template parameter!
template<char const *sz>
struct greeting
{
    std::string say_hello() const
    {
        return sz;
    }
};

struct push_char
{
    push_char(std::string &str)
      : str_(&str)
    {}

    void operator()(char ch) const
    {
        this->str_->push_back(ch);
    }

    std::string *str_;
};

int main()
{
    // Test mpl::size of strings
    {
        typedef mpl::string<'aaaa','aaaa','aaaa','aaaa','aaaa','aaaa','aaaa','aaa'> almost_full;
        typedef mpl::string<'aaaa','aaaa','aaaa','aaaa','aaaa','aaaa','aaaa','aaaa'> full;

        BOOST_MPL_ASSERT_RELATION(0,  ==, (mpl::size<mpl::string<> >::value));
        BOOST_MPL_ASSERT_RELATION(1,  ==, (mpl::size<mpl::string<'a'> >::value));
        BOOST_MPL_ASSERT_RELATION(2,  ==, (mpl::size<mpl::string<'ab'> >::value));
        BOOST_MPL_ASSERT_RELATION(2,  ==, (mpl::size<mpl::string<'a','b'> >::value));
        BOOST_MPL_ASSERT_RELATION(4,  ==, (mpl::size<mpl::string<'abcd'> >::value));
        BOOST_MPL_ASSERT_RELATION(5,  ==, (mpl::size<mpl::string<'abcd','e'> >::value));
        BOOST_MPL_ASSERT_RELATION(31, ==, (mpl::size<almost_full>::value));
        BOOST_MPL_ASSERT_RELATION(32, ==, (mpl::size<full>::value));
    }

    // Test mpl::begin and mpl::end with strings
    {
        typedef mpl::string<'aaaa','aaaa','aaaa','aaaa','aaaa','aaaa','aaaa','aaa'> almost_full;
        typedef mpl::string<'aaaa','aaaa','aaaa','aaaa','aaaa','aaaa','aaaa','aaaa'> full;

        BOOST_MPL_ASSERT((
            boost::is_same<
                mpl::begin<mpl::string<> >::type
              , mpl::end<mpl::string<> >::type
            >
        ));

        BOOST_MPL_ASSERT((
            boost::is_same<
                mpl::begin<mpl::string<'a'> >::type
              , mpl::string_iterator<mpl::string<'a'>, 0, 0>
            >
        ));

        BOOST_MPL_ASSERT((
            boost::is_same<
                mpl::end<mpl::string<'a'> >::type
              , mpl::string_iterator<mpl::string<'a'>, 1, 0>
            >
        ));

        BOOST_MPL_ASSERT((
            boost::is_same<
                mpl::begin<almost_full>::type
              , mpl::string_iterator<almost_full, 0, 0>
            >
        ));

        BOOST_MPL_ASSERT((
            boost::is_same<
                mpl::end<almost_full>::type
              , mpl::string_iterator<almost_full, 8, 0>
            >
        ));

        BOOST_MPL_ASSERT((
            boost::is_same<
                mpl::begin<full>::type
              , mpl::string_iterator<full, 0, 0>
            >
        ));

        BOOST_MPL_ASSERT((
            boost::is_same<
                mpl::end<full>::type
              , mpl::string_iterator<full, 8, 0>
            >
        ));
    }

    // testing push_back
    {
        typedef mpl::push_back<mpl::string<>, mpl::char_<'a'> >::type t1;
        BOOST_MPL_ASSERT((boost::is_same<t1, mpl::string<'a'> >));

        typedef mpl::push_back<t1, mpl::char_<'b'> >::type t2;
        BOOST_MPL_ASSERT((boost::is_same<t2, mpl::string<'ab'> >));

        typedef mpl::push_back<t2, mpl::char_<'c'> >::type t3;
        BOOST_MPL_ASSERT((boost::is_same<t3, mpl::string<'abc'> >));

        typedef mpl::push_back<t3, mpl::char_<'d'> >::type t4;
        BOOST_MPL_ASSERT((boost::is_same<t4, mpl::string<'abcd'> >));

        typedef mpl::push_back<t4, mpl::char_<'e'> >::type t5;
        BOOST_MPL_ASSERT((boost::is_same<t5, mpl::string<'abcd','e'> >));

        typedef mpl::string<'aaaa','aaaa','aaaa','aaaa','aaaa','aaaa','aaaa','aaa'> almost_full;
        typedef mpl::push_back<almost_full, mpl::char_<'X'> >::type t6;
        BOOST_MPL_ASSERT((boost::is_same<t6, mpl::string<'aaaa','aaaa','aaaa','aaaa','aaaa','aaaa','aaaa','aaaX'> >));
    }

    // Test mpl::next
    {
        typedef mpl::string<'a','bc','def','ghij'> s;

        typedef mpl::begin<s>::type i0;
        BOOST_MPL_ASSERT((boost::is_same<i0, mpl::string_iterator<s,0,0> >));

        typedef mpl::next<i0>::type i1;
        BOOST_MPL_ASSERT((boost::is_same<i1, mpl::string_iterator<s,1,0> >));

        typedef mpl::next<i1>::type i2;
        BOOST_MPL_ASSERT((boost::is_same<i2, mpl::string_iterator<s,1,1> >));

        typedef mpl::next<i2>::type i3;
        BOOST_MPL_ASSERT((boost::is_same<i3, mpl::string_iterator<s,2,0> >));

        typedef mpl::next<i3>::type i4;
        BOOST_MPL_ASSERT((boost::is_same<i4, mpl::string_iterator<s,2,1> >));

        typedef mpl::next<i4>::type i5;
        BOOST_MPL_ASSERT((boost::is_same<i5, mpl::string_iterator<s,2,2> >));

        typedef mpl::next<i5>::type i6;
        BOOST_MPL_ASSERT((boost::is_same<i6, mpl::string_iterator<s,3,0> >));

        typedef mpl::next<i6>::type i7;
        BOOST_MPL_ASSERT((boost::is_same<i7, mpl::string_iterator<s,3,1> >));

        typedef mpl::next<i7>::type i8;
        BOOST_MPL_ASSERT((boost::is_same<i8, mpl::string_iterator<s,3,2> >));

        typedef mpl::next<i8>::type i9;
        BOOST_MPL_ASSERT((boost::is_same<i9, mpl::string_iterator<s,3,3> >));

        typedef mpl::next<i9>::type i10;
        BOOST_MPL_ASSERT((boost::is_same<i10, mpl::string_iterator<s,4,0> >));

        BOOST_MPL_ASSERT((boost::is_same<i10, mpl::end<s>::type>));
    }

    // Test mpl::prior
    {
        typedef mpl::string<'a','bc','def','ghij'> s;

        typedef mpl::end<s>::type i10;
        BOOST_MPL_ASSERT((boost::is_same<i10, mpl::string_iterator<s,4,0> >));

        typedef mpl::prior<i10>::type i9;
        BOOST_MPL_ASSERT((boost::is_same<i9, mpl::string_iterator<s,3,3> >));

        typedef mpl::prior<i9>::type i8;
        BOOST_MPL_ASSERT((boost::is_same<i8, mpl::string_iterator<s,3,2> >));

        typedef mpl::prior<i8>::type i7;
        BOOST_MPL_ASSERT((boost::is_same<i7, mpl::string_iterator<s,3,1> >));

        typedef mpl::prior<i7>::type i6;
        BOOST_MPL_ASSERT((boost::is_same<i6, mpl::string_iterator<s,3,0> >));

        typedef mpl::prior<i6>::type i5;
        BOOST_MPL_ASSERT((boost::is_same<i5, mpl::string_iterator<s,2,2> >));

        typedef mpl::prior<i5>::type i4;
        BOOST_MPL_ASSERT((boost::is_same<i4, mpl::string_iterator<s,2,1> >));

        typedef mpl::prior<i4>::type i3;
        BOOST_MPL_ASSERT((boost::is_same<i3, mpl::string_iterator<s,2,0> >));

        typedef mpl::prior<i3>::type i2;
        BOOST_MPL_ASSERT((boost::is_same<i2, mpl::string_iterator<s,1,1> >));

        typedef mpl::prior<i2>::type i1;
        BOOST_MPL_ASSERT((boost::is_same<i1, mpl::string_iterator<s,1,0> >));

        typedef mpl::prior<i1>::type i0;
        BOOST_MPL_ASSERT((boost::is_same<i0, mpl::string_iterator<s,0,0> >));

        BOOST_MPL_ASSERT((boost::is_same<i0, mpl::begin<s>::type>));
    }

    // Test mpl::deref
    {
        typedef mpl::string<'a','bc','def','ghij'> s;

        typedef mpl::begin<s>::type i0;
        BOOST_MPL_ASSERT((boost::is_same<mpl::deref<i0>::type, mpl::char_<'a'> >));

        typedef mpl::next<i0>::type i1;
        BOOST_MPL_ASSERT((boost::is_same<mpl::deref<i1>::type, mpl::char_<'b'> >));

        typedef mpl::next<i1>::type i2;
        BOOST_MPL_ASSERT((boost::is_same<mpl::deref<i2>::type, mpl::char_<'c'> >));

        typedef mpl::next<i2>::type i3;
        BOOST_MPL_ASSERT((boost::is_same<mpl::deref<i3>::type, mpl::char_<'d'> >));

        typedef mpl::next<i3>::type i4;
        BOOST_MPL_ASSERT((boost::is_same<mpl::deref<i4>::type, mpl::char_<'e'> >));

        typedef mpl::next<i4>::type i5;
        BOOST_MPL_ASSERT((boost::is_same<mpl::deref<i5>::type, mpl::char_<'f'> >));

        typedef mpl::next<i5>::type i6;
        BOOST_MPL_ASSERT((boost::is_same<mpl::deref<i6>::type, mpl::char_<'g'> >));

        typedef mpl::next<i6>::type i7;
        BOOST_MPL_ASSERT((boost::is_same<mpl::deref<i7>::type, mpl::char_<'h'> >));

        typedef mpl::next<i7>::type i8;
        BOOST_MPL_ASSERT((boost::is_same<mpl::deref<i8>::type, mpl::char_<'i'> >));

        typedef mpl::next<i8>::type i9;
        BOOST_MPL_ASSERT((boost::is_same<mpl::deref<i9>::type, mpl::char_<'j'> >));
    }

    // testing push_back
    {
        typedef mpl::push_back<mpl::string<>, mpl::char_<'a'> >::type t1;
        BOOST_MPL_ASSERT((boost::is_same<t1, mpl::string<'a'> >));

        typedef mpl::push_back<t1, mpl::char_<'b'> >::type t2;
        BOOST_MPL_ASSERT((boost::is_same<t2, mpl::string<'ab'> >));

        typedef mpl::push_back<t2, mpl::char_<'c'> >::type t3;
        BOOST_MPL_ASSERT((boost::is_same<t3, mpl::string<'abc'> >));

        typedef mpl::push_back<t3, mpl::char_<'d'> >::type t4;
        BOOST_MPL_ASSERT((boost::is_same<t4, mpl::string<'abcd'> >));

        typedef mpl::push_back<t4, mpl::char_<'e'> >::type t5;
        BOOST_MPL_ASSERT((boost::is_same<t5, mpl::string<'abcd','e'> >));

        typedef mpl::string<'aaaa','aaaa','aaaa','aaaa','aaaa','aaaa','aaaa','aaa'> almost_full;
        typedef mpl::push_back<almost_full, mpl::char_<'X'> >::type t6;
        BOOST_MPL_ASSERT((boost::is_same<t6, mpl::string<'aaaa','aaaa','aaaa','aaaa','aaaa','aaaa','aaaa','aaaX'> >));

        typedef mpl::string<'a','a','a','a','a','a','a','aaaa'> must_repack;
        typedef mpl::push_back<must_repack, mpl::char_<'X'> >::type t7;
        BOOST_MPL_ASSERT((boost::is_same<t7, mpl::string<'aaaa','aaaa','aaaX'> >));
    }

    BOOST_MPL_ASSERT((mpl::empty<mpl::string<> >));
    BOOST_MPL_ASSERT_NOT((mpl::empty<mpl::string<'hi!'> >));

    // testing push_front
    {
        typedef mpl::push_front<mpl::string<>, mpl::char_<'a'> >::type t1;
        BOOST_MPL_ASSERT((boost::is_same<t1, mpl::string<'a'> >));

        typedef mpl::push_front<t1, mpl::char_<'b'> >::type t2;
        BOOST_MPL_ASSERT((boost::is_same<t2, mpl::string<'ba'> >));

        typedef mpl::push_front<t2, mpl::char_<'c'> >::type t3;
        BOOST_MPL_ASSERT((boost::is_same<t3, mpl::string<'cba'> >));

        typedef mpl::push_front<t3, mpl::char_<'d'> >::type t4;
        BOOST_MPL_ASSERT((boost::is_same<t4, mpl::string<'dcba'> >));

        typedef mpl::push_front<t4, mpl::char_<'e'> >::type t5;
        BOOST_MPL_ASSERT((boost::is_same<t5, mpl::string<'e','dcba'> >));

        typedef mpl::string<'aaa','aaaa','aaaa','aaaa','aaaa','aaaa','aaaa','aaaa'> almost_full;
        typedef mpl::push_front<almost_full, mpl::char_<'X'> >::type t6;
        BOOST_MPL_ASSERT((boost::is_same<t6, mpl::string<'Xaaa','aaaa','aaaa','aaaa','aaaa','aaaa','aaaa','aaaa'> >));

        typedef mpl::string<'aaaa','a','a','a','a','a','a','a'> must_repack;
        typedef mpl::push_front<must_repack, mpl::char_<'X'> >::type t7;
        BOOST_MPL_ASSERT((boost::is_same<t7, mpl::string<'Xaaa','aaaa','aaaa'> >));
    }

    // Test c_str<>
    BOOST_TEST(0 == std::strcmp(
        mpl::c_str<mpl::string<> >::value
                             , ""
    ));

    BOOST_TEST(0 == std::strcmp(
        mpl::c_str<mpl::string<'Hell','o wo','rld!'> >::value
                             , "Hell" "o wo" "rld!"
    ));

    BOOST_TEST(0 == std::strcmp(
        mpl::c_str<mpl::string<'aaaa','aaaa','aaaa','aaaa','aaaa','aaaa','aaaa','aaaX'> >::value
                             , "aaaa" "aaaa" "aaaa" "aaaa" "aaaa" "aaaa" "aaaa" "aaaX"
    ));

    // test using a string as a template parameter
    greeting<mpl::c_str<mpl::string<'Hell','o wo','rld!'> >::value> g;
    BOOST_TEST("Hello world!" == g.say_hello());

    std::string result;
    mpl::for_each<mpl::string<'Hell','o wo','rld!'> >(push_char(result));
    BOOST_TEST("Hello world!" == result);

    BOOST_TEST(('h' == mpl::front<mpl::string<'hi!'> >::type()));
    BOOST_TEST(('!' == mpl::back<mpl::string<'hi!'> >::type()));

    // back-inserter with copy
    typedef mpl::vector_c<char, 'a','b','c','d','e'> rgc;
    BOOST_TEST(0 == std::strcmp("abcde", mpl::c_str<rgc>::value));
    typedef mpl::copy<rgc, mpl::back_inserter<mpl::string<> > >::type str;
    BOOST_TEST(0 == std::strcmp("abcde", mpl::c_str<str>::value));

    // test insert_range and erase
    {
        typedef mpl::string<'Hell','o wo','rld!'> hello;
        typedef mpl::advance_c<mpl::begin<hello>::type, 5>::type where;
        typedef mpl::string<' cru','el'> cruel;
        typedef mpl::insert_range<hello, where, cruel>::type hello_cruel;
        BOOST_TEST(0 == std::strcmp("Hello cruel world!", mpl::c_str<hello_cruel>::value));

        typedef mpl::erase<hello, mpl::begin<hello>::type, where>::type erased1;
        BOOST_TEST(0 == std::strcmp(" world!", mpl::c_str<erased1>::value));
    }

    // test pop_front
    {
        BOOST_MPL_ASSERT((
            boost::is_same<
                mpl::pop_front<mpl::string<'a'> >::type
              , mpl::string<>
            >
        ));

        BOOST_MPL_ASSERT((
            boost::is_same<
                mpl::pop_front<mpl::string<'ab'> >::type
              , mpl::string<'b'>
            >
        ));

        BOOST_MPL_ASSERT((
            boost::is_same<
                mpl::pop_front<mpl::string<'abc'> >::type
              , mpl::string<'bc'>
            >
        ));

        BOOST_MPL_ASSERT((
            boost::is_same<
                mpl::pop_front<mpl::string<'abcd'> >::type
              , mpl::string<'bcd'>
            >
        ));

        BOOST_MPL_ASSERT((
            boost::is_same<
                mpl::pop_front<mpl::string<'abcd','e'> >::type
              , mpl::string<'bcd','e'>
            >
        ));

        BOOST_MPL_ASSERT((
            boost::is_same<
                mpl::pop_front<mpl::string<'d','e'> >::type
              , mpl::string<'e'>
            >
        ));

        BOOST_MPL_ASSERT((
            boost::is_same<
                mpl::pop_front<mpl::string<'aaaa','aaaa','aaaa','aaaa','aaaa','aaaa','aaaa','aaaa'> >::type
              , mpl::string<'aaa','aaaa','aaaa','aaaa','aaaa','aaaa','aaaa','aaaa'>
            >
        ));
    }

    // test pop_back
    {
        BOOST_MPL_ASSERT((
            boost::is_same<
                mpl::pop_back<mpl::string<'a'> >::type
              , mpl::string<>
            >
        ));

        BOOST_MPL_ASSERT((
            boost::is_same<
                mpl::pop_back<mpl::string<'ab'> >::type
              , mpl::string<'a'>
            >
        ));

        BOOST_MPL_ASSERT((
            boost::is_same<
                mpl::pop_back<mpl::string<'abc'> >::type
              , mpl::string<'ab'>
            >
        ));

        BOOST_MPL_ASSERT((
            boost::is_same<
                mpl::pop_back<mpl::string<'abcd'> >::type
              , mpl::string<'abc'>
            >
        ));

        BOOST_MPL_ASSERT((
            boost::is_same<
                mpl::pop_back<mpl::string<'abcd','e'> >::type
              , mpl::string<'abcd'>
            >
        ));

        BOOST_MPL_ASSERT((
            boost::is_same<
                mpl::pop_back<mpl::string<'d','e'> >::type
              , mpl::string<'d'>
            >
        ));

        BOOST_MPL_ASSERT((
            boost::is_same<
                mpl::pop_back<mpl::string<'aaaa','aaaa','aaaa','aaaa','aaaa','aaaa','aaaa','aaaa'> >::type
              , mpl::string<'aaaa','aaaa','aaaa','aaaa','aaaa','aaaa','aaaa','aaa'>
            >
        ));
    }

    {
        BOOST_TEST((
            mpl::at_c<
                mpl::string<'\x7f'>
              , 0
            >::type::value == (char)0x7f
        ));

        BOOST_TEST((
            mpl::at_c<
                mpl::string<'\x80'>
              , 0
            >::type::value == (char)0x80
        ));

        BOOST_TEST((
            mpl::at_c<
                mpl::string<
                    mpl::at_c<
                        mpl::string<'\x7f'>
                      , 0
                    >::type::value
                >
              , 0
            >::type::value == (char)0x7f
        ));

        BOOST_TEST((
            mpl::at_c<
                mpl::string<
                    mpl::at_c<
                        mpl::string<'\x80'>
                      , 0
                    >::type::value
                >
              , 0
            >::type::value == (char)0x80
        ));
    }

    return boost::report_errors();
}
