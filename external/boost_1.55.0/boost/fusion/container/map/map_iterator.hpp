/*=============================================================================
    Copyright (c) 2005-2013 Joel de Guzman
    Copyright (c) 2005-2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_MAP_ITERATOR_02042013_0835)
#define BOOST_FUSION_MAP_ITERATOR_02042013_0835

#include <boost/fusion/iterator/iterator_facade.hpp>
#include <boost/mpl/minus.hpp>
#include <boost/mpl/equal_to.hpp>

namespace boost { namespace fusion
{
    struct random_access_traversal_tag;

    template <typename Seq, int Pos>
    struct map_iterator
        : iterator_facade<
            map_iterator<Seq, Pos>
          , typename Seq::category>
    {
        typedef Seq sequence;
        typedef mpl::int_<Pos> index;

        map_iterator(Seq& seq)
            : seq_(seq)
        {}

        template<typename Iterator>
        struct value_of
        {
            typedef typename Iterator::sequence sequence;
            typedef typename Iterator::index index;
            typedef
                decltype(std::declval<sequence>().get_val(index()))
            type;
        };

        template<typename Iterator>
        struct value_of_data
        {
            typedef typename Iterator::sequence sequence;
            typedef typename Iterator::index index;
            typedef
                decltype(std::declval<sequence>().get_val(index()).second)
            type;
        };

        template<typename Iterator>
        struct key_of
        {
            typedef typename Iterator::sequence sequence;
            typedef typename Iterator::index index;
            typedef
                decltype(std::declval<sequence>().get_key(index()))
            type;
        };

        template<typename Iterator>
        struct deref
        {
            typedef typename Iterator::sequence sequence;
            typedef typename Iterator::index index;
            typedef
                decltype(std::declval<sequence>().get(index()))
            type;

            static type
            call(Iterator const& it)
            {
                return it.seq_.get(typename Iterator::index());
            }
        };

        template<typename Iterator>
        struct deref_data
        {
            typedef typename Iterator::sequence sequence;
            typedef typename Iterator::index index;
            typedef
                decltype(std::declval<sequence>().get(index()).second)
            type;

            static type
            call(Iterator const& it)
            {
                return it.seq_.get(typename Iterator::index()).second;
            }
        };

        template <typename Iterator, typename N>
        struct advance
        {
            typedef typename Iterator::index index;
            typedef typename Iterator::sequence sequence;
            typedef map_iterator<sequence, index::value + N::value> type;

            static type
            call(Iterator const& i)
            {
                return type(i.seq_);
            }
        };

        template<typename Iterator>
        struct next
            : advance<Iterator, mpl::int_<1> >
        {};

        template<typename Iterator>
        struct prior
            : advance<Iterator, mpl::int_<-1> >
        {};

        template <typename I1, typename I2>
        struct distance
        {
            typedef typename
                mpl::minus<
                    typename I2::index, typename I1::index
                >::type
            type;

            static type
            call(I1 const&, I2 const&)
            {
                return type();
            }
        };

        template<typename I1, typename I2>
        struct equal_to
            : mpl::equal_to<typename I1::index, typename I2::index>
        {};

        Seq& seq_;

    private:
        // silence MSVC warning C4512: assignment operator could not be generated
        map_iterator& operator= (map_iterator const&);
    };

}}

#endif
