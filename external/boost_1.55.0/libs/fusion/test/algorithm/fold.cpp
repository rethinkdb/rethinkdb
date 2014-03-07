/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2007 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/container/vector/vector.hpp>
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/fusion/sequence/io/out.hpp>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/algorithm/iteration/fold.hpp>
#include <boost/fusion/algorithm/iteration/accumulate.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/next.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/vector.hpp>

#include <boost/type_traits/remove_const.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/is_reference.hpp>

#include <string>

using boost::mpl::if_;
using boost::mpl::int_;
using boost::is_same;

struct add_ints_only
{
    template<typename T>
    struct result;

    template <typename State, typename T>
    struct result<add_ints_only(State, T)>
    {
        typedef typename boost::remove_const<
            typename boost::remove_reference<State>::type>::type type;
    };

    template <typename State, typename T>
    State
    operator()(State const& state, T const& /*x*/) const
    {
        return state;
    }

    int
    operator()(int state, int x) const
    {
        return x + state;
    }
};

struct count_ints
{
    template<typename T>
    struct result;

    template <typename CountT, typename T>
    struct result<count_ints(CountT, T)>
    {
        typedef typename boost::remove_const<
            typename boost::remove_reference<CountT>::type>::type state;
        typedef typename boost::remove_const<
            typename boost::remove_reference<T>::type>::type elem;

        typedef typename
            if_<
                is_same<elem, int>
              , typename boost::mpl::next<state>::type
              , state
            >::type
        type;
    };

    template <typename CountT, typename T>
    typename result<count_ints(CountT, T)>::type
    operator()(CountT const&, T const&) const
    {
        typedef typename result<count_ints(CountT, T)>::type result_;
        return result_();
    }
};

struct appender
{
    typedef std::string result_type;

    std::string operator()(std::string const& str, char c) const
    {
        return str + c;
    }
};

struct lvalue_adder
{
    template<typename Sig>
    struct result;

    template<typename T0, typename T1>
    struct result<lvalue_adder(T0, T1&)>
    {
        // Second argument still needs to support rvalues - see definition of fusion::fold
        typedef T1 type;
    };

    template<typename T0, typename T1>
    T1 operator()(T0 const& lhs, T1& rhs) const
    {
        return lhs + rhs;
    }
};

int add(int lhs, int rhs)
{
    return lhs + rhs;
}

int
main()
{
    using namespace boost::fusion;
    namespace fusion = boost::fusion;

    {
        typedef vector<int, char, int, double> vector_type;
        vector_type v(12345, 'x', 678910, 3.36);
        int result = fold(v, 0, add_ints_only());
        std::cout << result << std::endl;
        BOOST_TEST(result == 12345+678910);
    }

    {
        typedef vector<int> vector_type;
        vector_type v(12345);

        int n = fusion::fold(v, int_<0>(), count_ints());
        std::cout << n << std::endl;
        BOOST_TEST(n == 1);
    }

    {
        typedef vector<int, char, int, double, int> vector_type;
        vector_type v(12345, 'x', 678910, 3.36, 8756);

        int n = fusion::fold(v, int_<0>(), count_ints());
        std::cout << n << std::endl;
        BOOST_TEST(n == 3);
    }

    {
        typedef boost::mpl::vector<int, char, int, double, int> mpl_vec;
        int n = fusion::fold(mpl_vec(), int_<0>(), count_ints());
        std::cout << n << std::endl;
        BOOST_TEST(n == 3);
    }

    {
        BOOST_TEST(fusion::fold(fusion::make_vector('a','b','c','d','e'), std::string(""), appender())
                   == "abcde");
    }

    {
        vector<int, int> vec(1,2);
        BOOST_TEST(fusion::fold(vec, 0, lvalue_adder()) == 3);
    }

    {
        vector<int, int> vec(1,2);
        BOOST_TEST(fusion::fold(vec, 0, add) == 3);
    }

    {
        typedef vector<int, char, int, double> vector_type;
        vector_type v(12345, 'x', 678910, 3.36);
        int result = accumulate(v, 0, add_ints_only());
        std::cout << result << std::endl;
        BOOST_TEST(result == 12345+678910);
    }

    {
        typedef vector<int> vector_type;
        vector_type v(12345);

        int n = fusion::accumulate(v, int_<0>(), count_ints());
        std::cout << n << std::endl;
        BOOST_TEST(n == 1);
    }

    {
        typedef vector<int, char, int, double, int> vector_type;
        vector_type v(12345, 'x', 678910, 3.36, 8756);

        int n = fusion::accumulate(v, int_<0>(), count_ints());
        std::cout << n << std::endl;
        BOOST_TEST(n == 3);
    }

    {
        typedef boost::mpl::vector<int, char, int, double, int> mpl_vec;
        int n = fusion::accumulate(mpl_vec(), int_<0>(), count_ints());
        std::cout << n << std::endl;
        BOOST_TEST(n == 3);
    }

    {
        BOOST_TEST(fusion::accumulate(fusion::make_vector('a','b','c','d','e'), std::string(""), appender())
                   == "abcde");
    }

    {
        vector<int, int> vec(1,2);
        BOOST_TEST(fusion::accumulate(vec, 0, add) == 3);
    }

    return boost::report_errors();
}

