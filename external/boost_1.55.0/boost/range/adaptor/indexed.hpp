// Boost.Range library
//
//  Copyright Thorsten Ottosen, Neil Groves 2006 - 2008. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#ifndef BOOST_RANGE_ADAPTOR_INDEXED_IMPL_HPP
#define BOOST_RANGE_ADAPTOR_INDEXED_IMPL_HPP

#include <boost/config.hpp>
#ifdef BOOST_MSVC
#pragma warning( push )
#pragma warning( disable : 4355 )
#endif

#include <boost/range/adaptor/argument_fwd.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/iterator/iterator_adaptor.hpp>



namespace boost
{
    namespace adaptors
    {
        // This structure exists to carry the parameters from the '|' operator
        // to the index adapter. The expression rng | indexed(1) instantiates
        // this structure and passes it as the right-hand operand to the
        // '|' operator.
        struct indexed
        {
            explicit indexed(std::size_t x) : val(x) {}
            std::size_t val;
        };
    }

    namespace range_detail
    {
        template< class Iter >
        class indexed_iterator
            : public boost::iterator_adaptor< indexed_iterator<Iter>, Iter >
        {
        private:
            typedef boost::iterator_adaptor< indexed_iterator<Iter>, Iter >
                  base;

            typedef BOOST_DEDUCED_TYPENAME base::difference_type index_type;

            index_type m_index;

        public:
            indexed_iterator()
            : m_index(index_type()) {}
            
            explicit indexed_iterator( Iter i, index_type index )
            : base(i), m_index(index)
            {
                BOOST_ASSERT( m_index >= 0 && "Indexed Iterator out of bounds" );
            }

            index_type index() const
            {
                return m_index;
            }

         private:
            friend class boost::iterator_core_access;

            void increment()
            {
                ++m_index;
                ++(this->base_reference());
            }


            void decrement()
            {
                BOOST_ASSERT( m_index > 0 && "Indexed Iterator out of bounds" );
                --m_index;
                --(this->base_reference());
            }

            void advance( index_type n )
            {
                m_index += n;
                BOOST_ASSERT( m_index >= 0 && "Indexed Iterator out of bounds" );
                this->base_reference() += n;
            }
        };

        template< class Rng >
        struct indexed_range :
            iterator_range< indexed_iterator<BOOST_DEDUCED_TYPENAME range_iterator<Rng>::type> >
        {
        private:
            typedef indexed_iterator<BOOST_DEDUCED_TYPENAME range_iterator<Rng>::type>
                iter_type;
            typedef iterator_range<iter_type>
                base;
        public:
            template< class Index >
            indexed_range( Index i, Rng& r )
              : base( iter_type(boost::begin(r), i), iter_type(boost::end(r),i) )
            { }
        };

    } // 'range_detail'

    // Make this available to users of this library. It will sometimes be
    // required since it is the return type of operator '|' and
    // index().
    using range_detail::indexed_range;

    namespace adaptors
    {
        template< class SinglePassRange >
        inline indexed_range<SinglePassRange>
        operator|( SinglePassRange& r,
                   const indexed& f )
        {
            return indexed_range<SinglePassRange>( f.val, r );
        }

        template< class SinglePassRange >
        inline indexed_range<const SinglePassRange>
        operator|( const SinglePassRange& r,
                   const indexed& f )
        {
            return indexed_range<const SinglePassRange>( f.val, r );
        }

        template<class SinglePassRange, class Index>
        inline indexed_range<SinglePassRange>
        index(SinglePassRange& rng, Index index_value)
        {
            return indexed_range<SinglePassRange>(index_value, rng);
        }

        template<class SinglePassRange, class Index>
        inline indexed_range<const SinglePassRange>
        index(const SinglePassRange& rng, Index index_value)
        {
            return indexed_range<const SinglePassRange>(index_value, rng);
        }
    } // 'adaptors'

}

#ifdef BOOST_MSVC
#pragma warning( pop )
#endif

#endif
