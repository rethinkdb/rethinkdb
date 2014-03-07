/*=============================================================================
    Copyright (c) 2010 Christopher Schmidt

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#include <boost/config.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/fusion/iterator.hpp>
#include <boost/fusion/algorithm/transformation/reverse.hpp>
#include <boost/fusion/algorithm/iteration/fold.hpp>
#include <boost/fusion/algorithm/iteration/reverse_fold.hpp>
#include <boost/fusion/algorithm/iteration/iter_fold.hpp>
#include <boost/fusion/algorithm/iteration/reverse_iter_fold.hpp>
#include <boost/fusion/container/vector/convert.hpp>
#include <boost/fusion/container/vector/vector.hpp>
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/fusion/support/pair.hpp>
#include <boost/fusion/mpl.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/back.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/range_c.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/back_inserter.hpp>
#include <boost/mpl/always.hpp>
#include <boost/mpl/copy.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <iostream>

namespace mpl=boost::mpl;
namespace fusion=boost::fusion;

#ifdef BOOST_FUSION_TEST_REVERSE_FOLD
#   ifdef BOOST_FUSION_TEST_ITER_FOLD
#       define BOOST_FUSION_TEST_FOLD_NAME reverse_iter_fold
#   else
#       define BOOST_FUSION_TEST_FOLD_NAME reverse_fold
#   endif
#else
#   ifdef BOOST_FUSION_TEST_ITER_FOLD
#       define BOOST_FUSION_TEST_FOLD_NAME iter_fold
#   else
#       define BOOST_FUSION_TEST_FOLD_NAME fold
#   endif
#endif

struct sum
{
    template<typename Sig>
    struct result;

    template<typename Self, typename State, typename T>
    struct result<Self(State,T)>
      : boost::fusion::result_of::make_pair<
            mpl::int_<
                boost::remove_reference<
                    State
                >::type::first_type::value+1
            >
          , int
        >
    {
        BOOST_MPL_ASSERT((typename boost::is_reference<State>::type));
        BOOST_MPL_ASSERT((typename boost::is_reference<T>::type));
    };

#ifdef BOOST_FUSION_TEST_ITER_FOLD
    template<typename State, typename It>
    typename result<sum const&(State const&,It const&)>::type
    operator()(State const& state, It const& it)const
    {
        static const int n=State::first_type::value;
        return fusion::make_pair<mpl::int_<n+1> >(
            state.second+fusion::deref(it)*n);
    }
#else
    template<typename State>
    typename result<sum const&(State const&, int const&)>::type
    operator()(State const& state, int const& e)const
    {
        static const int n=State::first_type::value;
        return fusion::make_pair<mpl::int_<n+1> >(state.second+e*n);
    }
#endif
};

struct meta_sum
{
    template<typename Sig>
    struct result;

    template<typename Self, typename State, typename T>
    struct result<Self(State,T)>
    {
        BOOST_MPL_ASSERT((typename boost::is_reference<State>::type));
        BOOST_MPL_ASSERT((typename boost::is_reference<T>::type));

        typedef typename boost::remove_reference<State>::type state;
        static const int n=mpl::front<state>::type::value;

#ifdef BOOST_FUSION_TEST_ITER_FOLD
        typedef typename
            boost::fusion::result_of::value_of<
                typename boost::remove_reference<T>::type
            >::type
        t;
#else
        typedef typename boost::remove_reference<T>::type t;
#endif

        typedef
            mpl::vector<
                mpl::int_<n+1>
              , mpl::int_<
                    mpl::back<state>::type::value+t::value*n
                >
            >
        type;
    };

    template<typename State, typename T>
    typename result<meta_sum const&(State const&,T const&)>::type
    operator()(State const&, T const&)const;
};

struct fold_test_n
{
    template<typename I>
    void
    operator()(I)const
    {
        static const int n=I::value;
        typedef mpl::range_c<int, 0, n> range;

        static const int squares_sum=n*(n+1)*(2*n+1)/6;

        {
            mpl::range_c<int, 1, n+1> init_range;
            typename boost::fusion::result_of::as_vector<
                typename mpl::transform<
                    range
                  , mpl::always<int>
                  , mpl::back_inserter<mpl::vector<> >
                >::type
            >::type vec(
#ifdef BOOST_FUSION_TEST_REVERSE_FOLD
                fusion::reverse(init_range)
#else
                init_range
#endif
            );

            int result=BOOST_FUSION_TEST_FOLD_NAME(
                vec,
                fusion::make_pair<mpl::int_<1> >(0),
                sum()).second;
            std::cout << n << ": " << result << std::endl;
            BOOST_TEST(result==squares_sum);
        }

        {
            typedef typename
#ifdef BOOST_FUSION_TEST_REVERSE_FOLD
                boost::fusion::result_of::as_vector<
                    typename mpl::copy<
                        mpl::range_c<int, 1, n+1>
                      , mpl::front_inserter<fusion::vector<> >
                    >::type
                >::type
#else
                boost::fusion::result_of::as_vector<mpl::range_c<int, 1, n+1> >::type
#endif
            vec;

            typedef
                boost::is_same<
                    typename boost::fusion::result_of::BOOST_FUSION_TEST_FOLD_NAME<
                        vec
                      , mpl::vector<mpl::int_<1>, mpl::int_<0> >
                      , meta_sum
                    >::type
                  , typename mpl::if_c<
                        !n
                      , mpl::vector<mpl::int_<1>, mpl::int_<0> > const&
                      , mpl::vector<mpl::int_<n+1>, mpl::int_<squares_sum> >
                    >::type
                >
            result_test;

            BOOST_MPL_ASSERT((result_test));
        }
    }
};

int
main()
{
    mpl::for_each<mpl::range_c<int, 0, 10> >(fold_test_n());

    return boost::report_errors();
}

#undef BOOST_FUSION_TEST_FOLD_NAME

