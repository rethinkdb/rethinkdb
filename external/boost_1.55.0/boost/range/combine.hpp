//  Copyright Neil Groves 2010. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_COMBINE_HPP
#define BOOST_RANGE_COMBINE_HPP

#include <boost/iterator/zip_iterator.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/range/iterator.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/type_traits/is_void.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/plus.hpp>
#include <boost/mpl/arithmetic.hpp>
#include <boost/config.hpp>

namespace boost
{
    namespace range_detail
    {
        struct void_ { typedef void_ type; };
    }

    template<> struct range_iterator< ::boost::range_detail::void_ >
    {
       typedef ::boost::tuples::null_type type;
    };

    namespace range_detail
    {
        inline ::boost::tuples::null_type range_begin( ::boost::range_detail::void_& )
        { return ::boost::tuples::null_type(); }

        inline ::boost::tuples::null_type range_begin( const ::boost::range_detail::void_& )
        { return ::boost::tuples::null_type(); }

        inline ::boost::tuples::null_type range_end( ::boost::range_detail::void_& )
        { return ::boost::tuples::null_type(); }

        inline ::boost::tuples::null_type range_end( const ::boost::range_detail::void_& )
        { return ::boost::tuples::null_type(); }

        template< class T >
        struct tuple_iter
        {
            typedef BOOST_DEDUCED_TYPENAME ::boost::mpl::eval_if_c<
                ::boost::is_same<T, ::boost::range_detail::void_ >::value,
                ::boost::mpl::identity< ::boost::tuples::null_type >,
                ::boost::range_iterator<T>
            >::type type;
        };

        template< class Rng1, class Rng2 >
        struct tuple_range
        {
            typedef BOOST_DEDUCED_TYPENAME ::boost::mpl::eval_if_c<
                ::boost::is_same<Rng1, ::boost::range_detail::void_ >::value,
                ::boost::range_detail::void_,
                ::boost::mpl::identity<Rng1>
            >::type type;
        };

        template
        <
            class R1,
            class R2,
            class R3,
            class R4,
            class R5,
            class R6
        >
        struct generate_tuple
        {
            typedef ::boost::tuples::tuple<
                        BOOST_DEDUCED_TYPENAME tuple_iter<R1>::type,
                        BOOST_DEDUCED_TYPENAME tuple_iter<R2>::type,
                        BOOST_DEDUCED_TYPENAME tuple_iter<R3>::type,
                        BOOST_DEDUCED_TYPENAME tuple_iter<R4>::type,
                        BOOST_DEDUCED_TYPENAME tuple_iter<R5>::type,
                        BOOST_DEDUCED_TYPENAME tuple_iter<R6>::type
                    > type;

            static type begin( R1& r1, R2& r2, R3& r3, R4& r4, R5& r5, R6& r6 )
            {
                return ::boost::tuples::make_tuple( ::boost::begin(r1),
                                                    ::boost::begin(r2),
                                                    ::boost::begin(r3),
                                                    ::boost::begin(r4),
                                                    ::boost::begin(r5),
                                                    ::boost::begin(r6) );
            }

            static type end( R1& r1, R2& r2, R3& r3, R4& r4, R5& r5, R6& r6 )
            {
                return ::boost::tuples::make_tuple( ::boost::end(r1),
                                                    ::boost::end(r2),
                                                    ::boost::end(r3),
                                                    ::boost::end(r4),
                                                    ::boost::end(r5),
                                                    ::boost::end(r6) );
            }
        };

        template
        <
            class R1,
            class R2 = void_,
            class R3 = void_,
            class R4 = void_,
            class R5 = void_,
            class R6 = void_
        >
        struct zip_rng
            : iterator_range<
                zip_iterator<
                    BOOST_DEDUCED_TYPENAME generate_tuple<R1,R2,R3,R4,R5,R6>::type
                >
            >
        {
        private:
            typedef generate_tuple<R1,R2,R3,R4,R5,R6>        generator_t;
            typedef BOOST_DEDUCED_TYPENAME generator_t::type tuple_t;
            typedef zip_iterator<tuple_t>                    zip_iter_t;
            typedef iterator_range<zip_iter_t>               base_t;

        public:
            zip_rng( R1& r1, R2& r2, R3& r3, R4& r4, R5& r5, R6& r6 )
            : base_t( zip_iter_t( generator_t::begin(r1,r2,r3,r4,r5,r6) ),
                      zip_iter_t( generator_t::end(r1,r2,r3,r4,r5,r6) ) )
            {
                BOOST_ASSERT(::boost::distance(r1) <= ::boost::distance(r2));
                BOOST_ASSERT(::boost::distance(r1) <= ::boost::distance(r3));
                BOOST_ASSERT(::boost::distance(r1) <= ::boost::distance(r4));
                BOOST_ASSERT(::boost::distance(r1) <= ::boost::distance(r5));
                BOOST_ASSERT(::boost::distance(r1) <= ::boost::distance(r6));
            }

            template< class Zip, class Rng >
            zip_rng( Zip& z, Rng& r )
            : base_t( zip_iter_t( generator_t::begin( z, r ) ),
                      zip_iter_t( generator_t::end( z, r ) ) )
            {

                // @todo: tuple::begin( should be overloaded for this situation
            }

            struct tuple_length : ::boost::tuples::length<tuple_t>
            { };

            template< unsigned N >
            struct get
            {
                template< class Z, class R >
                static BOOST_DEDUCED_TYPENAME ::boost::tuples::element<N,tuple_t>::type begin( Z& z, R& )
                {
                    return get<N>( z.begin().get_iterator_tuple() );
                }

                template< class Z, class R >
                static BOOST_DEDUCED_TYPENAME ::boost::tuples::element<N,tuple_t>::type end( Z& z, R& r )
                {
                    return get<N>( z.end().get_iterator_tuple() );
                }
            };

        };

        template< class Rng1, class Rng2 >
        struct zip_range
            : iterator_range<
                zip_iterator<
                    ::boost::tuples::tuple<
                        BOOST_DEDUCED_TYPENAME ::boost::range_iterator<Rng1>::type,
                        BOOST_DEDUCED_TYPENAME ::boost::range_iterator<Rng2>::type
                    >
                >
            >
        {
        private:
            typedef zip_iterator<
                        ::boost::tuples::tuple<
                            BOOST_DEDUCED_TYPENAME ::boost::range_iterator<Rng1>::type,
                            BOOST_DEDUCED_TYPENAME ::boost::range_iterator<Rng2>::type
                        >
                    > zip_iter_t;
            typedef iterator_range<zip_iter_t> base_t;

        public:
            zip_range( Rng1& r1, Rng2& r2 )
            : base_t( zip_iter_t( ::boost::tuples::make_tuple(::boost::begin(r1),
                                                              ::boost::begin(r2)) ),
                      zip_iter_t( ::boost::tuples::make_tuple(::boost::end(r1),
                                                              ::boost::end(r2)) ) )
            {
                BOOST_ASSERT(::boost::distance(r1) <= ::boost::distance(r2));
            }
        };

        template< class Rng1, class Rng2, class Rng3 >
        struct zip_range3
            : iterator_range<
                zip_iterator<
                    ::boost::tuples::tuple<
                        BOOST_DEDUCED_TYPENAME ::boost::range_iterator<Rng1>::type,
                        BOOST_DEDUCED_TYPENAME ::boost::range_iterator<Rng2>::type,
                        BOOST_DEDUCED_TYPENAME ::boost::range_iterator<Rng3>::type
                    >
                >
            >
        {
        private:
            typedef zip_iterator<
                ::boost::tuples::tuple<
                    BOOST_DEDUCED_TYPENAME ::boost::range_iterator<Rng1>::type,
                    BOOST_DEDUCED_TYPENAME ::boost::range_iterator<Rng2>::type,
                    BOOST_DEDUCED_TYPENAME ::boost::range_iterator<Rng3>::type
                >
            > zip_iter_t;
            typedef iterator_range<zip_iter_t> base_t;

        public:
            zip_range3( Rng1& r1, Rng2& r2, Rng3& r3 )
            : base_t( zip_iter_t( ::boost::tuples::make_tuple(::boost::begin(r1),
                                                              ::boost::begin(r2),
                                                              ::boost::begin(r3)) ),
                      zip_iter_t( ::boost::tuples::make_tuple(::boost::end(r1),
                                                              ::boost::end(r2),
                                                              ::boost::end(r3)) )
                    )
            {
                BOOST_ASSERT(::boost::distance(r1) <= ::boost::distance(r2));
                BOOST_ASSERT(::boost::distance(r1) <= ::boost::distance(r3));
            }
        };


        struct combine_tag {};

        template< class Rng >
        inline zip_rng<Rng>
        operator&( combine_tag, Rng& r )
        {
            return zip_rng<Rng>(r);
        }

        template< class Rng >
        inline iterator_range<const Rng>
        operator&( combine_tag, const Rng& r )
        {
            return iterator_range<const Rng>(r);
        }

        template
        <
            class R1,
            class R2,
            class R3,
            class R4,
            class R5,
            class Rng
        >
        inline BOOST_DEDUCED_TYPENAME zip_rng<R1,R2,R3,R4,R5>::next
        operator&( const zip_rng<R1,R2,R3,R4,R5>& zip,
                   Rng& r )
        {
            return zip_rng<R1,R2,R3,R4,R5>::next( zip, r );
        }

    } // namespace range_detail

    template< class Rng1, class Rng2 >
    inline ::boost::range_detail::zip_range<Rng1, Rng2> combine( Rng1& r1, Rng2& r2 )
    {
        return ::boost::range_detail::zip_range<Rng1, Rng2>(r1, r2);
    }

    template< class Rng1, class Rng2 >
    inline ::boost::range_detail::zip_range<const Rng1, Rng2> combine( const Rng1& r1, Rng2& r2 )
    {
        return ::boost::range_detail::zip_range<const Rng1, Rng2>(r1, r2);
    }

    template< class Rng1, class Rng2 >
    inline ::boost::range_detail::zip_range<Rng1, const Rng2> combine( Rng1& r1, const Rng2& r2 )
    {
        return ::boost::range_detail::zip_range<Rng1, const Rng2>(r1, r2);
    }

    template< class Rng1, class Rng2 >
    inline ::boost::range_detail::zip_range<const Rng1, const Rng2> combine( const Rng1& r1, const Rng2& r2 )
    {
        return ::boost::range_detail::zip_range<const Rng1, const Rng2>(r1, r2);
    }

} // namespace boost

#endif
