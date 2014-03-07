// Boost.Range library
//
//  Copyright Neil Groves 2007. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#ifndef BOOST_RANGE_ADAPTOR_REPLACED_IF_IMPL_HPP_INCLUDED
#define BOOST_RANGE_ADAPTOR_REPLACED_IF_IMPL_HPP_INCLUDED

#include <boost/config.hpp>
#include <boost/range/adaptor/argument_fwd.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/value_type.hpp>
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/iterator/transform_iterator.hpp>

namespace boost
{
    namespace range_detail
    {
        template< class Pred, class Value >
        class replace_value_if
        {
        public:
            typedef const Value& result_type;
            typedef const Value& first_argument_type;

            replace_value_if(const Pred& pred, const Value& to)
                :   m_pred(pred), m_to(to)
            {
            }

            const Value& operator()(const Value& x) const
            {
                return m_pred(x) ? m_to : x;
            }

        private:
            Pred  m_pred;
            Value m_to;
        };

        template< class Pred, class R >
        class replaced_if_range :
            public boost::iterator_range<
                boost::transform_iterator<
                    replace_value_if< Pred, BOOST_DEDUCED_TYPENAME range_value<R>::type >,
                    BOOST_DEDUCED_TYPENAME range_iterator<R>::type > >
        {
        private:
            typedef replace_value_if< Pred, BOOST_DEDUCED_TYPENAME range_value<R>::type > Fn;

            typedef boost::iterator_range<
                boost::transform_iterator<
                    replace_value_if< Pred, BOOST_DEDUCED_TYPENAME range_value<R>::type >,
                    BOOST_DEDUCED_TYPENAME range_iterator<R>::type > > base_t;

        public:
            typedef BOOST_DEDUCED_TYPENAME range_value<R>::type value_type;

            replaced_if_range( R& r, const Pred& pred, value_type to )
                : base_t( make_transform_iterator( boost::begin(r), Fn(pred, to) ),
                          make_transform_iterator( boost::end(r), Fn(pred, to) ) )
            { }
        };

        template< class Pred, class T >
        class replace_if_holder
        {
        public:
            replace_if_holder( const Pred& pred, const T& to )
                : m_pred(pred), m_to(to)
            { }

            const Pred& pred() const { return m_pred; }
            const T& to() const { return m_to; }

        private:
            Pred m_pred;
            T m_to;
        };

        template< class Pred, class InputRng >
        inline replaced_if_range<Pred, InputRng>
        operator|( InputRng& r,
                   const replace_if_holder<Pred, BOOST_DEDUCED_TYPENAME range_value<InputRng>::type>& f )
        {
            return replaced_if_range<Pred, InputRng>(r, f.pred(), f.to());
        }

        template< class Pred, class InputRng >
        inline replaced_if_range<Pred, const InputRng>
        operator|( const InputRng& r,
                   const replace_if_holder<Pred, BOOST_DEDUCED_TYPENAME range_value<InputRng>::type>& f )
        {
            return replaced_if_range<Pred, const InputRng>(r, f.pred(), f.to());
        }
    } // 'range_detail'

    using range_detail::replaced_if_range;

    namespace adaptors
    {
        namespace
        {
            const range_detail::forwarder2TU<range_detail::replace_if_holder>
                replaced_if =
                    range_detail::forwarder2TU<range_detail::replace_if_holder>();
        }
        
        template<class Pred, class InputRange>
        inline replaced_if_range<Pred, InputRange>
        replace_if(InputRange& rng, Pred pred,
                   BOOST_DEDUCED_TYPENAME range_value<InputRange>::type to)
        {
            return range_detail::replaced_if_range<Pred, InputRange>(rng, pred, to);
        }

        template<class Pred, class InputRange>
        inline replaced_if_range<Pred, const InputRange>
        replace_if(const InputRange& rng, Pred pred,
                   BOOST_DEDUCED_TYPENAME range_value<const InputRange>::type to)
        {
            return range_detail::replaced_if_range<Pred, const InputRange>(rng, pred, to);
        }
    } // 'adaptors'
    
} // 'boost'

#endif // include guard
