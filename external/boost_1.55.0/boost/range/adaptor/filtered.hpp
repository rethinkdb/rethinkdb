// Boost.Range library
//
//  Copyright Thorsten Ottosen, Neil Groves 2006 - 2008. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#ifndef BOOST_RANGE_ADAPTOR_FILTERED_HPP
#define BOOST_RANGE_ADAPTOR_FILTERED_HPP

#include <boost/range/adaptor/argument_fwd.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/iterator/filter_iterator.hpp>

namespace boost
{
    namespace range_detail
    {
        template< class P, class R >
        struct filtered_range :
            boost::iterator_range<
                boost::filter_iterator< P,
                    BOOST_DEDUCED_TYPENAME range_iterator<R>::type
                >
            >
        {
        private:
            typedef boost::iterator_range<
                        boost::filter_iterator< P,
                            BOOST_DEDUCED_TYPENAME range_iterator<R>::type
                        >
                    > base;
        public:
            filtered_range( P p, R& r )
            : base( make_filter_iterator( p, boost::begin(r), boost::end(r) ),
                    make_filter_iterator( p, boost::end(r), boost::end(r) ) )
            { }
        };

        template< class T >
        struct filter_holder : holder<T>
        {
            filter_holder( T r ) : holder<T>(r)
            { }
        };

        template< class InputRng, class Predicate >
        inline filtered_range<Predicate, InputRng>
        operator|( InputRng& r,
                   const filter_holder<Predicate>& f )
        {
            return filtered_range<Predicate, InputRng>( f.val, r );
        }

        template< class InputRng, class Predicate >
        inline filtered_range<Predicate, const InputRng>
        operator|( const InputRng& r,
                   const filter_holder<Predicate>& f )
        {
            return filtered_range<Predicate, const InputRng>( f.val, r );
        }

    } // 'range_detail'

    // Unusual use of 'using' is intended to bring filter_range into the boost namespace
    // while leaving the mechanics of the '|' operator in range_detail and maintain
    // argument dependent lookup.
    // filter_range logically needs to be in the boost namespace to allow user of
    // the library to define the return type for filter()
    using range_detail::filtered_range;

    namespace adaptors
    {
        namespace
        {
            const range_detail::forwarder<range_detail::filter_holder>
                    filtered =
                       range_detail::forwarder<range_detail::filter_holder>();
        }

        template<class InputRange, class Predicate>
        inline filtered_range<Predicate, InputRange>
        filter(InputRange& rng, Predicate filter_pred)
        {
            return range_detail::filtered_range<Predicate, InputRange>( filter_pred, rng );
        }

        template<class InputRange, class Predicate>
        inline filtered_range<Predicate, const InputRange>
        filter(const InputRange& rng, Predicate filter_pred)
        {
            return range_detail::filtered_range<Predicate, const InputRange>( filter_pred, rng );
        }
    } // 'adaptors'

}

#endif
