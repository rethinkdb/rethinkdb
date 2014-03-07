/*=============================================================================
    Copyright (c) 2008 Francois Barel

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/type_traits/is_same.hpp>

#include <boost/spirit/include/qi_operator.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

#include <iterator>
#include "test.hpp"


namespace testns
{

    BOOST_SPIRIT_TERMINAL_NAME_EX( ops, ops_type )


    ///////////////////////////////////////////////////////////////////////////
    // Parsers
    ///////////////////////////////////////////////////////////////////////////

    template <typename T1>
    struct ops_1_parser
      : boost::spirit::qi::primitive_parser<ops_1_parser<T1> >
    {
        ops_1_parser(T1 t1)
          : t1(t1)
        {}

        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef int type;   // Number of parsed chars.
        };

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& /*context*/, Skipper const& skipper
          , Attribute& attr) const
        {
            boost::spirit::qi::skip_over(first, last, skipper);

            int count = 0;

            Iterator it = first;
            typedef typename std::iterator_traits<Iterator>::value_type Char;
            for (T1 t = 0; t < t1; t++, count++)
                if (it == last || *it++ != Char('+'))
                    return false;

            boost::spirit::traits::assign_to(count, attr);
            first = it;
            return true;
        }

        template <typename Context>
        boost::spirit::qi::info what(Context& /*context*/) const
        {
            return boost::spirit::qi::info("ops_1");
        }

        const T1 t1;

    private:
        // silence MSVC warning C4512: assignment operator could not be generated
        ops_1_parser& operator= (ops_1_parser const&);
    };

    template <typename T1, typename T2>
    struct ops_2_parser
      : boost::spirit::qi::primitive_parser<ops_2_parser<T1, T2> >
    {
        ops_2_parser(T1 t1, T2 t2)
          : t1(t1)
          , t2(t2)
        {}

        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef int type;   // Number of parsed chars.
        };

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& /*context*/, Skipper const& skipper
          , Attribute& attr) const
        {
            boost::spirit::qi::skip_over(first, last, skipper);

            int count = 0;

            Iterator it = first;
            typedef typename std::iterator_traits<Iterator>::value_type Char;
            for (T1 t = 0; t < t1; t++, count++)
                if (it == last || *it++ != Char('+'))
                    return false;
            for (T2 t = 0; t < t2; t++, count++)
                if (it == last || *it++ != Char('-'))
                    return false;

            boost::spirit::traits::assign_to(count, attr);
            first = it;
            return true;
        }

        template <typename Context>
        boost::spirit::qi::info what(Context& /*context*/) const
        {
            return boost::spirit::qi::info("ops_2");
        }

        const T1 t1;
        const T2 t2;

    private:
        // silence MSVC warning C4512: assignment operator could not be generated
        ops_2_parser& operator= (ops_2_parser const&);
    };

    template <typename T1, typename T2, typename T3>
    struct ops_3_parser
      : boost::spirit::qi::primitive_parser<ops_3_parser<T1, T2, T3> >
    {
        ops_3_parser(T1 t1, T2 t2, T3 t3)
          : t1(t1)
          , t2(t2)
          , t3(t3)
        {}

        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef int type;   // Number of parsed chars.
        };

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& /*context*/, Skipper const& skipper
          , Attribute& attr) const
        {
            boost::spirit::qi::skip_over(first, last, skipper);

            int count = 0;

            Iterator it = first;
            typedef typename std::iterator_traits<Iterator>::value_type Char;
            for (T1 t = 0; t < t1; t++, count++)
                if (it == last || *it++ != Char('+'))
                    return false;
            for (T2 t = 0; t < t2; t++, count++)
                if (it == last || *it++ != Char('-'))
                    return false;
            for (T3 t = 0; t < t3; t++, count++)
                if (it == last || *it++ != Char('*'))
                    return false;

            boost::spirit::traits::assign_to(count, attr);
            first = it;
            return true;
        }

        template <typename Context>
        boost::spirit::qi::info what(Context& /*context*/) const
        {
            return boost::spirit::qi::info("ops_3");
        }

        const T1 t1;
        const T2 t2;
        const T3 t3;

    private:
        // silence MSVC warning C4512: assignment operator could not be generated
        ops_3_parser& operator= (ops_3_parser const&);
    };

}


namespace boost { namespace spirit
{

    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////

    template <typename T1>
    struct use_terminal<qi::domain
      , terminal_ex<testns::tag::ops, fusion::vector1<T1> > >
      : mpl::true_ {};

    template <typename T1, typename T2>
    struct use_terminal<qi::domain
      , terminal_ex<testns::tag::ops, fusion::vector2<T1, T2> > >
      : mpl::true_ {};

    template <typename T1, typename T2, typename T3>
    struct use_terminal<qi::domain
      , terminal_ex<testns::tag::ops, fusion::vector3<T1, T2, T3> > >
      : mpl::true_ {};

    template <>
    struct use_lazy_terminal<qi::domain, testns::tag::ops, 1>
      : mpl::true_ {};

    template <>
    struct use_lazy_terminal<qi::domain, testns::tag::ops, 2>
      : mpl::true_ {};

    template <>
    struct use_lazy_terminal<qi::domain, testns::tag::ops, 3>
      : mpl::true_ {};

}}

namespace boost { namespace spirit { namespace qi
{

    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////

    template <typename Modifiers, typename T1>
    struct make_primitive<
        terminal_ex<testns::tag::ops, fusion::vector1<T1> >
      , Modifiers>
    {
        typedef testns::ops_1_parser<T1> result_type;
        template <typename Terminal>
        result_type operator()(const Terminal& term, unused_type) const
        {
            return result_type(
                fusion::at_c<0>(term.args)
            );
        }
    };

    template <typename Modifiers, typename T1, typename T2>
    struct make_primitive<
        terminal_ex<testns::tag::ops, fusion::vector2<T1, T2> >
      , Modifiers>
    {
        typedef testns::ops_2_parser<T1, T2> result_type;
        template <typename Terminal>
        result_type operator()(const Terminal& term, unused_type) const
        {
            return result_type(
                fusion::at_c<0>(term.args)
              , fusion::at_c<1>(term.args)
            );
        }
    };

    template <typename Modifiers, typename T1, typename T2, typename T3>
    struct make_primitive<
        terminal_ex<testns::tag::ops, fusion::vector3<T1, T2, T3> >
      , Modifiers>
    {
        typedef testns::ops_3_parser<T1, T2, T3> result_type;
        template <typename Terminal>
        result_type operator()(const Terminal& term, unused_type) const
        {
            return result_type(
                fusion::at_c<0>(term.args)
              , fusion::at_c<1>(term.args)
              , fusion::at_c<2>(term.args)
            );
        }
    };

}}}


namespace testns
{
    template <typename T1, typename T>
    void check_type_1(const T& /*t*/)
    {
        namespace fusion = boost::fusion;
        BOOST_STATIC_ASSERT(( boost::is_same<T
          , typename boost::spirit::terminal<testns::tag::ops>::result<T1>::type >::value ));
    }

    template <typename T1, typename T2, typename T>
    void check_type_2(const T& /*t*/)
    {
        namespace fusion = boost::fusion;
        BOOST_STATIC_ASSERT(( boost::is_same<T
          , typename boost::spirit::terminal<testns::tag::ops>::result<T1, T2>::type >::value ));
    }

    template <typename T1, typename T2, typename T3, typename T>
    void check_type_3(const T& /*t*/)
    {
        namespace fusion = boost::fusion;
        BOOST_STATIC_ASSERT(( boost::is_same<T
          , typename boost::spirit::terminal<testns::tag::ops>::result<T1, T2, T3>::type >::value ));
    }
}


int
main()
{
    using spirit_test::test_attr;
    using spirit_test::test;

    using testns::ops;
    using testns::check_type_1;
    using testns::check_type_2;
    using testns::check_type_3;

    { // immediate args
        int c = 0;
#define IP1 ops(2)
        check_type_1<int>(IP1);
        BOOST_TEST(test_attr("++/", IP1 >> '/', c) && c == 2);

        c = 0;
#define IP2 ops(2, 3)
        check_type_2<int, int>(IP2);
        BOOST_TEST(test_attr("++---/", IP2 >> '/', c) && c == 5);

        c = 0;
#define IP3 ops(2, 3, 4)
        check_type_3<int, int, int>(IP3);
        BOOST_TEST(!test("++---***/", IP3 >> '/'));
#define IP4 ops(2, 3, 4)
        check_type_3<int, int, int>(IP4);
        BOOST_TEST(test_attr("++---****/", IP4 >> '/', c) && c == 9);
    }

#ifndef BOOST_SPIRIT_USE_PHOENIX_V3

    using boost::phoenix::val;
    using boost::phoenix::actor;
    using boost::phoenix::value;

    { // all lazy args
        int c = 0;
#define LP1 ops(val(1))
        check_type_1<actor<value<int> > >(LP1);
        BOOST_TEST(test_attr("+/", LP1 >> '/', c) && c == 1);

        c = 0;
#define LP2 ops(val(1), val(4))
        check_type_2<actor<value<int> >, actor<value<int> > >(LP2);
        BOOST_TEST(test_attr("+----/", LP2 >> '/', c) && c == 5);

        c = 0;
#define LP3 ops(val((char)2), val(3.), val(4))
        check_type_3<actor<value<char> >, actor<value<double> >, actor<value<int> > >(LP3);
        BOOST_TEST(!test("++---***/", LP3 >> '/'));
#define LP4 ops(val(1), val(2), val(3))
        check_type_3<actor<value<int> >, actor<value<int> >, actor<value<int> > >(LP4);
        BOOST_TEST(test_attr("+--***/", LP4 >> '/', c) && c == 6);
    }

    { // mixed immediate and lazy args
        namespace fusion = boost::fusion;
        namespace phx = boost::phoenix;

        int c = 0;
#define MP1 ops(val(3), 2)
        check_type_2<actor<value<int> >, int>(MP1);
        BOOST_TEST(test_attr("+++--/", MP1 >> '/', c) && c == 5);

        c = 0;
#define MP2 ops(4, val(1))
        check_type_2<int, actor<value<int> > >(MP2);
        BOOST_TEST(test_attr("++++-/", MP2 >> '/', c) && c == 5);

        c = 0;
#define MP3 ops(2, val(2), val(2))
        check_type_3<int, actor<value<int> >, actor<value<int> > >(MP3);
        BOOST_TEST(!test("++-**/", MP3 >> '/'));
#define MP4 ops(2, val(2), 2)
        check_type_3<int, actor<value<int> >, int>(MP4);
        BOOST_TEST(test_attr("++--**/", MP4 >> '/', c) && c == 6);

        c = 0;
#define MP5 ops(val(5) - val(3), 2, val(2))
        check_type_3<actor<phx::composite<phx::minus_eval, fusion::vector<value<int>, value<int> > > >, int, actor<value<int> > >(MP5);
        BOOST_TEST(test_attr("++--**/", MP5 >> '/', c) && c == 6);
    }

#else // BOOST_SPIRIT_USE_PHOENIX_V3

    using boost::phoenix::val;
    using boost::phoenix::actor;
    using boost::phoenix::expression::value;

    { // all lazy args
        int c = 0;
#define LP1 ops(val(1))
        check_type_1<value<int>::type>(LP1);
        BOOST_TEST(test_attr("+/", LP1 >> '/', c) && c == 1);

        c = 0;
#define LP2 ops(val(1), val(4))
        check_type_2<value<int>::type, value<int>::type>(LP2);
        BOOST_TEST(test_attr("+----/", LP2 >> '/', c) && c == 5);

        c = 0;
#define LP3 ops(val((char)2), val(3.), val(4))
        check_type_3<value<char>::type, value<double>::type, value<int>::type>(LP3);
        BOOST_TEST(!test("++---***/", LP3 >> '/'));
#define LP4 ops(val(1), val(2), val(3))
        check_type_3<value<int>::type, value<int>::type, value<int>::type>(LP4);
        BOOST_TEST(test_attr("+--***/", LP4 >> '/', c) && c == 6);
    }

    { // mixed immediate and lazy args
        namespace fusion = boost::fusion;
        namespace phx = boost::phoenix;

        int c = 0;
#define MP1 ops(val(3), 2)
        check_type_2<value<int>::type, int>(MP1);
        BOOST_TEST(test_attr("+++--/", MP1 >> '/', c) && c == 5);

        c = 0;
#define MP2 ops(4, val(1))
        check_type_2<int, value<int>::type>(MP2);
        BOOST_TEST(test_attr("++++-/", MP2 >> '/', c) && c == 5);

        c = 0;
#define MP3 ops(2, val(2), val(2))
        check_type_3<int, value<int>::type, value<int>::type>(MP3);
        BOOST_TEST(!test("++-**/", MP3 >> '/'));
#define MP4 ops(2, val(2), 2)
        check_type_3<int, value<int>::type, int>(MP4);
        BOOST_TEST(test_attr("++--**/", MP4 >> '/', c) && c == 6);

        c = 0;
#define MP5 ops(val(5) - val(3), 2, val(2))
        check_type_3<phx::expression::minus<value<int>::type, value<int>::type>::type, int, value<int>::type>(MP5);
        BOOST_TEST(test_attr("++--**/", MP5 >> '/', c) && c == 6);
    }
#endif

    return boost::report_errors();
}

