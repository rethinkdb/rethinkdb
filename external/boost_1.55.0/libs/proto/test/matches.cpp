///////////////////////////////////////////////////////////////////////////////
// matches.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <string>
#include <iostream>
#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/proto/core.hpp>
#include <boost/proto/debug.hpp>
#include <boost/proto/transform/arg.hpp>
#include <boost/test/unit_test.hpp>

namespace mpl = boost::mpl;
namespace proto = boost::proto;
namespace fusion = boost::fusion;

struct int_convertible
{
    int_convertible() {}
    operator int() const { return 0; }
};

struct Input
  : proto::or_<
        proto::shift_right< proto::terminal< std::istream & >, proto::_ >
      , proto::shift_right< Input, proto::_ >
    >
{};

struct Output
  : proto::or_<
        proto::shift_left< proto::terminal< std::ostream & >, proto::_ >
      , proto::shift_left< Output, proto::_ >
    >
{};

proto::terminal< std::istream & >::type const cin_ = {std::cin};
proto::terminal< std::ostream & >::type const cout_ = {std::cout};

struct Anything
  : proto::or_<
        proto::terminal<proto::_>
      , proto::nary_expr<proto::_, proto::vararg<Anything> >
    >
{};

void a_function() {}

struct MyCases
{
    template<typename Tag>
    struct case_
      : proto::not_<proto::_>
    {};
};

template<>
struct MyCases::case_<proto::tag::shift_right>
  : proto::_
{};

template<>
struct MyCases::case_<proto::tag::plus>
  : proto::_
{};

enum binary_representation_enum
{
    magnitude
  , two_complement
};

typedef
    mpl::integral_c<binary_representation_enum, magnitude>
magnitude_c;

typedef
    mpl::integral_c<binary_representation_enum, two_complement>
two_complement_c;

template<typename Type, typename Representation>
struct number
{};

struct NumberGrammar
  : proto::or_ <
        proto::terminal<number<proto::_, two_complement_c> >
      , proto::terminal<number<proto::_, magnitude_c> >
    >
{};

struct my_terminal
{};

template<typename T>
struct a_template
{};

template<typename Expr>
struct my_expr;

struct my_domain
  : proto::domain<proto::pod_generator<my_expr> >
{};

template<typename Expr>
struct my_expr
{
    BOOST_PROTO_BASIC_EXTENDS(Expr, my_expr, my_domain)
};

void test_matches()
{
    proto::assert_matches< proto::_ >( proto::lit(1) );
    proto::assert_matches< proto::_ >( proto::as_child(1) );
    proto::assert_matches< proto::_ >( proto::as_expr(1) );

    proto::assert_matches< proto::terminal<int> >( proto::lit(1) );
    proto::assert_matches< proto::terminal<int> >( proto::as_child(1) );
    proto::assert_matches< proto::terminal<int> >( proto::as_expr(1) );

    proto::assert_matches_not< proto::terminal<int> >( proto::lit('a') );
    proto::assert_matches_not< proto::terminal<int> >( proto::as_child('a') );
    proto::assert_matches_not< proto::terminal<int> >( proto::as_expr('a') );

    proto::assert_matches< proto::terminal<proto::convertible_to<int> > >( proto::lit('a') );
    proto::assert_matches< proto::terminal<proto::convertible_to<int> > >( proto::as_child('a') );
    proto::assert_matches< proto::terminal<proto::convertible_to<int> > >( proto::as_expr('a') );

    proto::assert_matches_not< proto::terminal<int> >( proto::lit((int_convertible())) );
    proto::assert_matches_not< proto::terminal<int> >( proto::as_child((int_convertible())) );
    proto::assert_matches_not< proto::terminal<int> >( proto::as_expr((int_convertible())) );

    proto::assert_matches< proto::terminal<proto::convertible_to<int> > >( proto::lit((int_convertible())) );
    proto::assert_matches< proto::terminal<proto::convertible_to<int> > >( proto::as_child((int_convertible())) );
    proto::assert_matches< proto::terminal<proto::convertible_to<int> > >( proto::as_expr((int_convertible())) );

    proto::assert_matches< proto::if_<boost::is_same<proto::_value, int>() > >( proto::lit(1) );
    proto::assert_matches_not< proto::if_<boost::is_same<proto::_value, int>() > >( proto::lit('a') );

    proto::assert_matches<
        proto::and_<
            proto::terminal<proto::_>
          , proto::if_<boost::is_same<proto::_value, int>() >
        >
    >( proto::lit(1) );

    proto::assert_matches_not<
        proto::and_<
            proto::terminal<proto::_>
          , proto::if_<boost::is_same<proto::_value, int>() >
        >
    >( proto::lit('a') );

    proto::assert_matches< proto::terminal<char const *> >( proto::lit("hello") );
    proto::assert_matches< proto::terminal<char const *> >( proto::as_child("hello") );
    proto::assert_matches< proto::terminal<char const *> >( proto::as_expr("hello") );

    proto::assert_matches< proto::terminal<char const[6]> >( proto::lit("hello") );
    proto::assert_matches< proto::terminal<char const (&)[6]> >( proto::as_child("hello") );
    proto::assert_matches< proto::terminal<char const[6]> >( proto::as_expr("hello") );

    proto::assert_matches< proto::terminal<char [6]> >( proto::lit("hello") );
    proto::assert_matches< proto::terminal<char [6]> >( proto::as_child("hello") );
    proto::assert_matches< proto::terminal<char [6]> >( proto::as_expr("hello") );

    proto::assert_matches< proto::terminal<char const[proto::N]> >( proto::lit("hello") );
    proto::assert_matches< proto::terminal<char const (&)[proto::N]> >( proto::as_child("hello") );
    proto::assert_matches< proto::terminal<char const[proto::N]> >( proto::as_expr("hello") );

    proto::assert_matches< proto::terminal<char [proto::N]> >( proto::lit("hello") );
    proto::assert_matches< proto::terminal<char [proto::N]> >( proto::as_child("hello") );
    proto::assert_matches< proto::terminal<char [proto::N]> >( proto::as_expr("hello") );

    proto::assert_matches< proto::terminal<wchar_t const[proto::N]> >( proto::lit(L"hello") );
    proto::assert_matches< proto::terminal<wchar_t const (&)[proto::N]> >( proto::as_child(L"hello") );
    proto::assert_matches< proto::terminal<wchar_t const[proto::N]> >( proto::as_expr(L"hello") );

    proto::assert_matches< proto::terminal<wchar_t [proto::N]> >( proto::lit(L"hello") );
    proto::assert_matches< proto::terminal<wchar_t [proto::N]> >( proto::as_child(L"hello") );
    proto::assert_matches< proto::terminal<wchar_t [proto::N]> >( proto::as_expr(L"hello") );

    proto::assert_matches_not< proto::if_<boost::is_same<proto::_value, int>()> >( proto::lit("hello") );

    proto::assert_matches< proto::terminal<std::string> >( proto::lit(std::string("hello")) );
    proto::assert_matches< proto::terminal<std::string> >( proto::as_child(std::string("hello")) );
    proto::assert_matches< proto::terminal<std::string> >( proto::as_expr(std::string("hello")) );

    proto::assert_matches< proto::terminal<std::basic_string<proto::_> > >( proto::lit(std::string("hello")) );
    proto::assert_matches< proto::terminal<std::basic_string<proto::_> > >( proto::as_child(std::string("hello")) );
    proto::assert_matches< proto::terminal<std::basic_string<proto::_> > >( proto::as_expr(std::string("hello")) );

    proto::assert_matches_not< proto::terminal<std::basic_string<proto::_> > >( proto::lit(1) );
    proto::assert_matches_not< proto::terminal<std::basic_string<proto::_> > >( proto::as_child(1) );
    proto::assert_matches_not< proto::terminal<std::basic_string<proto::_> > >( proto::as_expr(1) );

    proto::assert_matches_not< proto::terminal<std::basic_string<proto::_,proto::_,proto::_> > >( proto::lit(1) );
    proto::assert_matches_not< proto::terminal<std::basic_string<proto::_,proto::_,proto::_> > >( proto::as_child(1) );
    proto::assert_matches_not< proto::terminal<std::basic_string<proto::_,proto::_,proto::_> > >( proto::as_expr(1) );

    #if BOOST_WORKAROUND(__HP_aCC, BOOST_TESTED_AT(61700))
    typedef std::string const const_string;
    #else
    typedef std::string const_string;
    #endif
    
    proto::assert_matches< proto::terminal<std::basic_string<proto::_> const & > >( proto::lit(const_string("hello")) );
    proto::assert_matches< proto::terminal<std::basic_string<proto::_> const & > >( proto::as_child(const_string("hello")) );
    proto::assert_matches_not< proto::terminal<std::basic_string<proto::_> const & > >( proto::as_expr(const_string("hello")) );

    proto::assert_matches< proto::terminal< void(&)() > >( proto::lit(a_function) );
    proto::assert_matches< proto::terminal< void(&)() > >( proto::as_child(a_function) );
    proto::assert_matches< proto::terminal< void(&)() > >( proto::as_expr(a_function) );

    proto::assert_matches_not< proto::terminal< void(*)() > >( proto::lit(a_function) );
    proto::assert_matches_not< proto::terminal< void(*)() > >( proto::as_child(a_function) );
    proto::assert_matches_not< proto::terminal< void(*)() > >( proto::as_expr(a_function) );

    proto::assert_matches< proto::terminal< proto::convertible_to<void(*)()> > >( proto::lit(a_function) );
    proto::assert_matches< proto::terminal< proto::convertible_to<void(*)()> > >( proto::as_child(a_function) );
    proto::assert_matches< proto::terminal< proto::convertible_to<void(*)()> > >( proto::as_expr(a_function) );

    proto::assert_matches< proto::terminal< void(*)() > >( proto::lit(&a_function) );
    proto::assert_matches< proto::terminal< void(*)() > >( proto::as_child(&a_function) );
    proto::assert_matches< proto::terminal< void(*)() > >( proto::as_expr(&a_function) );

    proto::assert_matches< proto::terminal< void(* const &)() > >( proto::lit(&a_function) );
    proto::assert_matches< proto::terminal< void(* const &)() > >( proto::as_child(&a_function) );
    proto::assert_matches_not< proto::terminal< void(* const &)() > >( proto::as_expr(&a_function) );

    proto::assert_matches<
        proto::or_<
            proto::if_<boost::is_same<proto::_value, char>() >
          , proto::if_<boost::is_same<proto::_value, int>() >
        >
    >( proto::lit(1) );

    proto::assert_matches_not<
        proto::or_<
            proto::if_<boost::is_same<proto::_value, char>() >
          , proto::if_<boost::is_same<proto::_value, int>() >
        >
    >( proto::lit(1u) );

    proto::assert_matches< Input >( cin_ >> 1 >> 2 >> 3 );
    proto::assert_matches_not< Output >( cin_ >> 1 >> 2 >> 3 );

    proto::assert_matches< Output >( cout_ << 1 << 2 << 3 );
    proto::assert_matches_not< Input >( cout_ << 1 << 2 << 3 );

    proto::assert_matches< proto::function< proto::terminal<int>, proto::vararg< proto::terminal<char> > > >( proto::lit(1)('a','b','c','d') );
    proto::assert_matches_not< proto::function< proto::terminal<int>, proto::vararg< proto::terminal<char> > > >( proto::lit(1)('a','b','c',"d") );

    proto::assert_matches< Anything >( cout_ << 1 << +proto::lit('a') << proto::lit(1)('a','b','c',"d") );

    proto::assert_matches< proto::switch_<MyCases> >( proto::lit(1) >> 'a' );
    proto::assert_matches< proto::switch_<MyCases> >( proto::lit(1) + 'a' );
    proto::assert_matches_not< proto::switch_<MyCases> >( proto::lit(1) << 'a' );

    number<int, two_complement_c> num;
    proto::assert_matches<NumberGrammar>(proto::as_expr(num));

    // check custom terminal types
    {
        proto::nullary_expr<my_terminal, int>::type i = {0};

        proto::assert_matches<proto::nullary_expr<my_terminal, proto::_> >( i );
        proto::assert_matches_not<proto::terminal<proto::_> >( i );

        proto::terminal<int>::type j = {0};
        proto::assert_matches<proto::terminal<proto::_> >( j );
        proto::assert_matches_not<proto::nullary_expr<my_terminal, proto::_> >( j );

        proto::assert_matches<proto::nullary_expr<proto::_, proto::_> >( i );
    }

    // check 0 and 1 arg forms or or_ and and_
    {
        proto::assert_matches< proto::and_<> >( proto::lit(1) );
        proto::assert_matches_not< proto::or_<> >( proto::lit(1) );

        proto::assert_matches< proto::and_<proto::terminal<int> > >( proto::lit(1) );
        proto::assert_matches< proto::or_<proto::terminal<int> > >( proto::lit(1) );
    }

    // Test lambda matches with arrays, a corner case that had
    // a bug that was reported by Antoine de Maricourt on boost@lists.boost.org
    {
        a_template<int[3]> a;
        proto::assert_matches< proto::terminal< a_template<proto::_> > >( proto::lit(a) );
    }

    // Test that the actual derived expression type makes it through to proto::if_
    {
        my_expr<proto::terminal<int>::type> e = {{1}};
        proto::assert_matches< proto::if_<boost::is_same<proto::domain_of<proto::_>, my_domain>()> >( e );
    }
}

using namespace boost::unit_test;
///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("test proto::matches<>");

    test->add(BOOST_TEST_CASE(&test_matches));

    return test;
}

