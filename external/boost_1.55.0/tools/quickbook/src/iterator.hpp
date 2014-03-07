/*=============================================================================
    Copyright (c) 2010 Daniel James

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_SPIRIT_QUICKBOOK_ITERATOR_HPP)
#define BOOST_SPIRIT_QUICKBOOK_ITERATOR_HPP

#include <boost/operators.hpp>
#include <boost/iterator/iterator_traits.hpp>
#include <boost/range/iterator_range.hpp>
#include <iterator>

namespace quickbook
{
    template <typename Iterator>
    struct lookback_iterator
        : boost::forward_iterator_helper<
            lookback_iterator<Iterator>,
            typename boost::iterator_value<Iterator>::type,
            typename boost::iterator_difference<Iterator>::type,
            typename boost::iterator_pointer<Iterator>::type,
            typename boost::iterator_reference<Iterator>::type
        >
    {
        lookback_iterator() {}
        explicit lookback_iterator(Iterator base)
            : original_(base), base_(base) {}
    
        friend bool operator==(
            lookback_iterator const& x,
            lookback_iterator const& y)
        {
            return x.base_ == y.base_;
        }
        
        lookback_iterator& operator++()
        {
            ++base_;
            return *this;
        }
    
        typename boost::iterator_reference<Iterator>::type operator*() const
        {
            return *base_;
        }
        
        Iterator base() const {
            return base_;
        }

        typedef boost::iterator_range<std::reverse_iterator<Iterator> >
            lookback_range;

        lookback_range lookback() const
        {
            return lookback_range(base_, original_);
        }
    
    private:
        Iterator original_;
        Iterator base_;
    };
}

#endif
