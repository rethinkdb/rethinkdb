// Boost.Range library
//
//  Copyright Neil Groves 2007. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_ADAPTOR_STRIDED_HPP_INCLUDED
#define BOOST_RANGE_ADAPTOR_STRIDED_HPP_INCLUDED

#include <boost/range/adaptor/argument_fwd.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/iterator/iterator_adaptor.hpp>
#include <iterator>

namespace boost
{
    namespace range_detail
    {
        // strided_iterator for wrapping a forward traversal iterator
        template<class BaseIterator, class Category>
        class strided_iterator
            : public iterator_adaptor<
                strided_iterator<BaseIterator, Category>
              , BaseIterator
              , use_default
              , boost::forward_traversal_tag
            >
        {
            friend class ::boost::iterator_core_access;

            typedef iterator_adaptor<
                        strided_iterator<BaseIterator, Category>
                      , BaseIterator
                      , use_default
                      , boost::forward_traversal_tag
                    > super_t;

        public:
            typedef BOOST_DEDUCED_TYPENAME std::iterator_traits<BaseIterator>::difference_type difference_type;
            typedef BaseIterator base_iterator;

            strided_iterator()
                : m_last()
                , m_stride()
            {
            }

            strided_iterator(base_iterator first, base_iterator it, base_iterator last, difference_type stride)
                : super_t(it)
                , m_last(last)
                , m_stride(stride)
            {
            }

            template<class OtherIterator>
            strided_iterator(const strided_iterator<OtherIterator, Category>& other,
                             BOOST_DEDUCED_TYPENAME enable_if_convertible<OtherIterator, base_iterator>::type* = 0)
                : super_t(other)
                , m_last(other.base_end())
                , m_stride(other.get_stride())
            {
            }

            base_iterator base_end() const { return m_last; }
            difference_type get_stride() const { return m_stride; }

        private:
            void increment()
            {
                base_iterator& it = this->base_reference();
                for (difference_type i = 0; (it != m_last) && (i < m_stride); ++i)
                    ++it;
            }

            base_iterator m_last;
            difference_type m_stride;
        };

        // strided_iterator for wrapping a bidirectional iterator
        template<class BaseIterator>
        class strided_iterator<BaseIterator, bidirectional_traversal_tag>
            : public iterator_adaptor<
                strided_iterator<BaseIterator, bidirectional_traversal_tag>
              , BaseIterator
              , use_default
              , bidirectional_traversal_tag
            >
        {
            friend class ::boost::iterator_core_access;

            typedef iterator_adaptor<
                        strided_iterator<BaseIterator, bidirectional_traversal_tag>
                      , BaseIterator
                      , use_default
                      , bidirectional_traversal_tag
                    > super_t;
        public:
            typedef BOOST_DEDUCED_TYPENAME std::iterator_traits<BaseIterator>::difference_type difference_type;
            typedef BaseIterator base_iterator;

            strided_iterator()
                : m_first()
                , m_last()
                , m_stride()
            {
            }

            strided_iterator(base_iterator first, base_iterator it, base_iterator last, difference_type stride)
                : super_t(it)
                , m_first(first)
                , m_last(last)
                , m_stride(stride)
            {
            }

            template<class OtherIterator>
            strided_iterator(const strided_iterator<OtherIterator, bidirectional_traversal_tag>& other,
                             BOOST_DEDUCED_TYPENAME enable_if_convertible<OtherIterator, base_iterator>::type* = 0)
                : super_t(other.base())
                , m_first(other.base_begin())
                , m_last(other.base_end())
                , m_stride(other.get_stride())
            {
            }

            base_iterator base_begin() const { return m_first; }
            base_iterator base_end() const { return m_last; }
            difference_type get_stride() const { return m_stride; }

        private:
            void increment()
            {
                base_iterator& it = this->base_reference();
                for (difference_type i = 0; (it != m_last) && (i < m_stride); ++i)
                    ++it;
            }

            void decrement()
            {
                base_iterator& it = this->base_reference();
                for (difference_type i = 0; (it != m_first) && (i < m_stride); ++i)
                    --it;
            }

            base_iterator m_first;
            base_iterator m_last;
            difference_type m_stride;
        };

        // strided_iterator implementation for wrapping a random access iterator
        template<class BaseIterator>
        class strided_iterator<BaseIterator, random_access_traversal_tag>
            : public iterator_adaptor<
                        strided_iterator<BaseIterator, random_access_traversal_tag>
                      , BaseIterator
                      , use_default
                      , random_access_traversal_tag
                    >
        {
            friend class ::boost::iterator_core_access;

            typedef iterator_adaptor<
                        strided_iterator<BaseIterator, random_access_traversal_tag>
                      , BaseIterator
                      , use_default
                      , random_access_traversal_tag
                    > super_t;
        public:
            typedef BOOST_DEDUCED_TYPENAME super_t::difference_type difference_type;
            typedef BaseIterator base_iterator;

            strided_iterator()
                : m_first()
                , m_last()
                , m_index(0)
                , m_stride()
            {
            }

            strided_iterator(BaseIterator first, BaseIterator it, BaseIterator last, difference_type stride)
                : super_t(it)
                , m_first(first)
                , m_last(last)
                , m_index(stride ? (it - first) / stride : 0)
                , m_stride(stride)
            {
            }

            template<class OtherIterator>
            strided_iterator(const strided_iterator<OtherIterator, random_access_traversal_tag>& other,
                             BOOST_DEDUCED_TYPENAME enable_if_convertible<OtherIterator, BaseIterator>::type* = 0)
                : super_t(other.base())
                , m_first(other.base_begin())
                , m_last(other.base_end())
                , m_index(other.get_index())
                , m_stride(other.get_stride())
            {
            }

            base_iterator base_begin() const { return m_first; }
            base_iterator base_end() const { return m_last; }
            difference_type get_stride() const { return m_stride; }
            difference_type get_index() const { return m_index; }

        private:
            void increment()
            {
                m_index += m_stride;
                if (m_index < (m_last - m_first))
                    this->base_reference() = m_first + m_index;
                else
                    this->base_reference() = m_last;
            }

            void decrement()
            {
                m_index -= m_stride;
                if (m_index >= 0)
                    this->base_reference() = m_first + m_index;
                else
                    this->base_reference() = m_first;
            }

            void advance(difference_type offset)
            {
                offset *= m_stride;
                m_index += offset;
                if (m_index < 0)
                    this->base_reference() = m_first;
                else if (m_index > (m_last - m_first))
                    this->base_reference() = m_last;
                else
                    this->base_reference() = m_first + m_index;
            }

            template<class OtherIterator>
            difference_type distance_to(const strided_iterator<OtherIterator, random_access_traversal_tag>& other,
                                        BOOST_DEDUCED_TYPENAME enable_if_convertible<OtherIterator, BaseIterator>::type* = 0) const
            {
                if (other.base() >= this->base())
                    return (other.base() - this->base() + (m_stride - 1)) / m_stride;
                return (other.base() - this->base() - (m_stride - 1)) / m_stride;
            }

            bool equal(const strided_iterator& other) const
            {
                return this->base() == other.base();
            }

        private:
            base_iterator m_first;
            base_iterator m_last;
            difference_type m_index;
            difference_type m_stride;
        };

        template<class BaseIterator, class Difference> inline
        strided_iterator<BaseIterator, BOOST_DEDUCED_TYPENAME iterator_traversal<BaseIterator>::type>
        make_strided_iterator(BaseIterator first, BaseIterator it,
                              BaseIterator last, Difference stride)
        {
            BOOST_ASSERT( stride >= 0 );
            typedef BOOST_DEDUCED_TYPENAME iterator_traversal<BaseIterator>::type traversal_tag;
            return strided_iterator<BaseIterator, traversal_tag>(first, it, last, stride);
        }

        template< class Rng
                , class Category = BOOST_DEDUCED_TYPENAME iterator_traversal<
                                    BOOST_DEDUCED_TYPENAME range_iterator<Rng>::type
                                   >::type
         >
        class strided_range
            : public iterator_range<
                        range_detail::strided_iterator<
                            BOOST_DEDUCED_TYPENAME range_iterator<Rng>::type,
                            Category
                        >
                     >
        {
            typedef range_detail::strided_iterator<
                        BOOST_DEDUCED_TYPENAME range_iterator<Rng>::type,
                        Category
                    > iter_type;
            typedef iterator_range<iter_type> super_t;
        public:
            template<class Difference>
            strided_range(Difference stride, Rng& rng)
                : super_t(make_strided_iterator(boost::begin(rng), boost::begin(rng), boost::end(rng), stride),
                          make_strided_iterator(boost::begin(rng), boost::end(rng), boost::end(rng), stride))
            {
                BOOST_ASSERT( stride >= 0 );
            }
        };

        template<class Difference>
        class strided_holder : public holder<Difference>
        {
        public:
            explicit strided_holder(Difference value) : holder<Difference>(value) {}
        };

        template<class Rng, class Difference>
        inline strided_range<Rng>
        operator|(Rng& rng, const strided_holder<Difference>& stride)
        {
            return strided_range<Rng>(stride.val, rng);
        }

        template<class Rng, class Difference>
        inline strided_range<const Rng>
        operator|(const Rng& rng, const strided_holder<Difference>& stride)
        {
            return strided_range<const Rng>(stride.val, rng);
        }

    } // namespace range_detail

    using range_detail::strided_range;

    namespace adaptors
    {

        namespace
        {
            const range_detail::forwarder<range_detail::strided_holder>
                strided = range_detail::forwarder<range_detail::strided_holder>();
        }

        template<class Range, class Difference>
        inline strided_range<Range>
        stride(Range& rng, Difference step)
        {
            return strided_range<Range>(step, rng);
        }

        template<class Range, class Difference>
        inline strided_range<const Range>
        stride(const Range& rng, Difference step)
        {
            return strided_range<const Range>(step, rng);
        }

    } // namespace 'adaptors'
} // namespace 'boost'

#endif
