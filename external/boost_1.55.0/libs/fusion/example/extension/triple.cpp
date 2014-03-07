/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2011 Nathan Ridge
    Copyright (c) 2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

/*=============================================================================
    An implementation of a std::pair like triple<T0, T1, T2>
    We use fusion::sequence_facade and fusion::iterator_facade
    to make our triple a fully conforming Boost.Fusion random
    traversal sequence.
==============================================================================*/

#include <boost/detail/lightweight_test.hpp>

#include <boost/fusion/sequence/sequence_facade.hpp>
#include <boost/fusion/iterator/iterator_facade.hpp>
#include <boost/fusion/sequence/intrinsic.hpp>
#include <boost/fusion/iterator.hpp>
#include <boost/fusion/support/category_of.hpp>
#include <boost/fusion/algorithm/iteration/fold.hpp>

#include <boost/mpl/int.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/minus.hpp>
#include <boost/mpl/assert.hpp>

#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/is_same.hpp>

#include <string>

namespace mpl = boost::mpl;
namespace fusion = boost::fusion;

namespace demo
{
    template<typename Seq, int N>
    struct triple_iterator
        : fusion::iterator_facade<triple_iterator<Seq, N>,
          fusion::random_access_traversal_tag>
    {
        typedef mpl::int_<N> index;
        typedef Seq sequence_type;

        triple_iterator(Seq& seq)
            : seq_(seq) {}

        Seq& seq_;

        template<typename T>
        struct value_of;

        template<typename Sq>
        struct value_of<triple_iterator<Sq, 0> >
            : mpl::identity<typename Sq::t0_type>
        {};

        template<typename Sq>
        struct value_of<triple_iterator<Sq, 1> >
            : mpl::identity<typename Sq::t1_type>
        {};

        template<typename Sq>
        struct value_of<triple_iterator<Sq, 2> >
            : mpl::identity<typename Sq::t2_type>
        {};

        template<typename T>
        struct deref;

        template <typename Sq>
        struct deref<triple_iterator<Sq, 0> >
        {
            typedef typename Sq::t0_type& type;

            static type
            call(triple_iterator<Sq, 0> const& iter)
            {
                return iter.seq_.t0;
            }
        };

        template <typename Sq>
        struct deref<triple_iterator<Sq, 0> const>
        {
            typedef typename Sq::t0_type const& type;

            static type
            call(triple_iterator<Sq, 0> const& iter)
            {
                return iter.seq_.t0;
            }
        };

        template <typename Sq>
        struct deref<triple_iterator<Sq, 1> >
        {
            typedef typename Sq::t1_type& type;

            static type
            call(triple_iterator<Sq, 1> const& iter)
            {
                return iter.seq_.t1;
            }
        };

        template <typename Sq>
        struct deref<triple_iterator<Sq, 1> const>
        {
            typedef typename Sq::t1_type const& type;

            static type
            call(triple_iterator<Sq, 1> const& iter)
            {
                return iter.seq_.t1;
            }
        };

        template <typename Sq>
        struct deref<triple_iterator<Sq, 2> >
        {
            typedef typename Sq::t2_type& type;

            static type
            call(triple_iterator<Sq, 2> const& iter)
            {
                return iter.seq_.t2;
            }
        };

        template <typename Sq>
        struct deref<triple_iterator<Sq, 2> const>
        {
            typedef typename Sq::t2_type const& type;

            static type
            call(triple_iterator<Sq, 2> const& iter)
            {
                return iter.seq_.t2;
            }
        };

        template<typename It>
        struct next
        {
            typedef triple_iterator<
                typename It::sequence_type, It::index::value + 1>
            type;

            static type call(It const& it)
            {
                return type(it.seq_);
            }
        };

        template<typename It>
        struct prior
        {
            typedef triple_iterator<
                typename It::sequence_type, It::index::value - 1>
            type;

            static type call(It const& it)
            {
                return type(it.seq_);
            }
        };

        template<typename It1, typename It2>
        struct distance
        {
            typedef typename mpl::minus<
              typename It2::index, typename It1::index>::type
            type;

            static type call(It1 const& it1, It2 const& it2)
            {
                return type();
            }
        };

        template<typename It, typename M>
        struct advance
        {
            typedef triple_iterator<
                typename It::sequence_type,
                It::index::value + M::value>
            type;

            static type call(It const& it)
            {
                return type(it.seq_);
            }
        };
    };

    template<typename T0, typename T1, typename T2>
    struct triple
        : fusion::sequence_facade<triple<T0, T1, T2>,
          fusion::random_access_traversal_tag>
    {
        triple(T0 const& t0, T1 const& t1, T2 const& t2)
            : t0(t0), t1(t1), t2(t2)
        {}

        template<typename Sq>
        struct begin
        {
            typedef demo::triple_iterator<Sq, 0> type;

            static type call(Sq& sq)
            {
                return type(sq);
            }
        };

        template<typename Sq>
        struct end
        {
            typedef demo::triple_iterator<Sq, 3> type;

            static type call(Sq& sq)
            {
                return type(sq);
            }
        };

        template<typename Sq>
        struct size
            : mpl::int_<3>
        {};

        template<typename Sq, typename N>
        struct value_at
            : value_at<Sq, mpl::int_<N::value> >
        {};

        template<typename Sq>
        struct value_at<Sq, mpl::int_<0> >
        {
            typedef typename Sq::t0_type type;
        };

        template<typename Sq>
        struct value_at<Sq, mpl::int_<1> >
        {
            typedef typename Sq::t1_type type;
        };

        template<typename Sq>
        struct value_at<Sq, mpl::int_<2> >
        {
            typedef typename Sq::t2_type type;
        };

        template<typename Sq, typename N>
        struct at
            : at<Sq, mpl::int_<N::value> >
        {};

        template<typename Sq>
        struct at<Sq, mpl::int_<0> >
        {
            typedef typename
                mpl::if_<
                    boost::is_const<Sq>
                  , typename Sq::t0_type const&
                  , typename Sq::t0_type&
                >::type
            type;

            static type call(Sq& sq)
            {
                return sq.t0;
            }
        };

        template<typename Sq>
        struct at<Sq, mpl::int_<1> >
        {
            typedef typename
                mpl::if_<
                    boost::is_const<Sq>
                  , typename Sq::t1_type const&
                  , typename Sq::t1_type&
                >::type
            type;

            static type call(Sq& sq)
            {
                return sq.t1;
            }
        };

        template<typename Sq>
        struct at<Sq, mpl::int_<2> >
        {
            typedef typename
                mpl::if_<
                    boost::is_const<Sq>
                  , typename Sq::t2_type const&
                  , typename Sq::t2_type&
                >::type
            type;

            static type call(Sq& sq)
            {
                return sq.t2;
            }
        };

        typedef T0 t0_type;
        typedef T1 t1_type;
        typedef T2 t2_type;

        T0 t0;
        T1 t1;
        T2 t2;
    };
}

struct modifying_fold_functor
{
    template <typename T>
    struct result
    {
        typedef bool type;
    };

    template <typename T>
    bool operator()(bool b, T&)
    {
        return b;
    }
};

struct nonmodifying_fold_functor
{
    template <typename T>
    struct result
    {
        typedef bool type;
    };

    template <typename T>
    bool operator()(bool b, const T&)
    {
        return b;
    }
};

int main()
{
    typedef demo::triple<int, char, std::string> my_triple;
    my_triple t(101, 'a', "hello");
    BOOST_TEST(*fusion::begin(t) == 101);
    BOOST_TEST(*fusion::next(fusion::begin(t)) == 'a');
    BOOST_TEST(*fusion::prior(fusion::end(t)) == "hello");
    BOOST_TEST(fusion::distance(fusion::begin(t), fusion::end(t)) == 3);
    BOOST_TEST(fusion::size(t) == 3);
    BOOST_MPL_ASSERT((boost::is_same<
        int, fusion::result_of::value_at_c<my_triple, 0>::type>));
    BOOST_MPL_ASSERT((boost::is_same<
      char, fusion::result_of::value_at_c<my_triple, 1>::type>));
    BOOST_MPL_ASSERT((boost::is_same<
        std::string, fusion::result_of::value_at_c<my_triple, 2>::type>));
    BOOST_TEST(fusion::at_c<0>(t) == 101);
    BOOST_TEST(fusion::at_c<1>(t) == 'a');
    BOOST_TEST(fusion::at_c<2>(t) == "hello");
    BOOST_TEST(fusion::fold(t, true, modifying_fold_functor()) == true);
    BOOST_TEST(fusion::fold(t, true, nonmodifying_fold_functor()) == true);
    return boost::report_errors();
}
