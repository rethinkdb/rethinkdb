// Copyright 2005 Daniel Wallin. 
// Copyright 2005 Joel de Guzman.
// Copyright 2005 Dan Marsden. 
// Copyright 2008 Hartmut Kaiser. 
//
// Use, modification and distribution is subject to the Boost Software 
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Modeled after range_ex, Copyright 2004 Eric Niebler

#ifndef PHOENIX_ALGORITHM_QUERYING_HPP
#define PHOENIX_ALGORITHM_QUERYING_HPP

#include <algorithm>

#include <boost/spirit/home/phoenix/stl/algorithm/detail/has_find.hpp>
#include <boost/spirit/home/phoenix/stl/algorithm/detail/has_lower_bound.hpp>
#include <boost/spirit/home/phoenix/stl/algorithm/detail/has_upper_bound.hpp>
#include <boost/spirit/home/phoenix/stl/algorithm/detail/has_equal_range.hpp>

#include <boost/spirit/home/phoenix/stl/algorithm/detail/begin.hpp>
#include <boost/spirit/home/phoenix/stl/algorithm/detail/end.hpp>
#include <boost/spirit/home/phoenix/stl/algorithm/detail/decay_array.hpp>

#include <boost/spirit/home/phoenix/function/function.hpp>

#include <boost/range/result_iterator.hpp>
#include <boost/range/difference_type.hpp>

namespace boost { namespace phoenix {
namespace impl
{
    struct find
    {
        template<class R, class T>
        struct result : range_result_iterator<R>
        {};

        template<class R, class T>
        typename result<R, T>::type execute(R& r, T const& x, mpl::true_) const
        {
            return r.find(x);
        }

        template<class R, class T>
        typename result<R, T>::type execute(R& r, T const& x, mpl::false_) const
        {
            return std::find(detail::begin_(r), detail::end_(r), x);
        }

        template<class R, class T>
        typename result<R, T>::type operator()(R& r, T const& x) const
        {
            return execute(r, x, has_find<R>());
        }
    };

    struct find_if
    {
        template<class R, class P>
        struct result : range_result_iterator<R>
        {};

        template<class R, class P>
        typename result<R, P>::type operator()(R& r, P p) const
        {
            return std::find_if(detail::begin_(r), detail::end_(r), p);
        }
    };

    struct find_end
    {
        template<class R, class R2, class P = void>
        struct result : range_result_iterator<R>
        {};

        template<class R, class R2>
        typename result<R, R2>::type operator()(R& r, R2& r2) const
        {
            return std::find_end(
                detail::begin_(r)
                , detail::end_(r)
                , detail::begin_(r2)
                , detail::end_(r2)
                );
        }

        template<class R, class R2, class P>
        typename result<R, R2, P>::type operator()(R& r, R2& r2, P p) const
        {
            return std::find_end(
                detail::begin_(r)
                , detail::end_(r)
                , detail::begin_(r2)
                , detail::end_(r2)
                , p
                );
        }
    };

    struct find_first_of
    {
        template<class R, class R2, class P = void>
        struct result : range_result_iterator<R>
        {};

        template<class R, class R2>
        typename result<R, R2>::type operator()(R& r, R2& r2) const
        {
            return std::find_first_of(
                detail::begin_(r)
                , detail::end_(r)
                , detail::begin_(r2)
                , detail::end_(r2)
                );
        }

        template<class R, class R2, class P>
        typename result<R, R2, P>::type operator()(R& r, R2& r2, P p) const
        {
            return std::find_first_of(
                detail::begin_(r)
                , detail::end_(r)
                , detail::begin_(r2)
                , detail::end_(r2)
                , p
                );
        }
    };

    struct adjacent_find
    {
        template<class R, class P = void>
        struct result : range_result_iterator<R>
        {};

        template<class R>
        typename result<R>::type operator()(R& r) const
        {
            return std::adjacent_find(detail::begin_(r), detail::end_(r));
        }

        template<class R, class P>
        typename result<R, P>::type operator()(R& r, P p) const
        {
            return std::adjacent_find(detail::begin_(r), detail::end_(r), p);
        }
    };

    struct count
    {
        template<class R, class T>
        struct result : range_difference<R>
        {};

        template<class R, class T>
        typename result<R, T>::type operator()(R& r, T const& x) const
        {
            return std::count(detail::begin_(r), detail::end_(r), x);
        }
    };

    struct count_if
    {
        template<class R, class P>
        struct result : range_difference<R>
        {};

        template<class R, class P>
        typename result<R, P>::type operator()(R& r, P p) const
        {
            return std::count_if(detail::begin_(r), detail::end_(r), p);
        }
    };

    struct distance
    {
        template<class R>
        struct result : range_difference<R>
        {};

        template<class R>
        typename result<R>::type operator()(R& r) const
        {
            return std::distance(detail::begin_(r), detail::end_(r));
        }
    };

    struct equal
    {
        template<class R, class I, class P = void>
        struct result
        {
            typedef bool type;
        };

        template<class R, class I>
        bool operator()(R& r, I i) const
        {
            return std::equal(detail::begin_(r), detail::end_(r), i);
        }

        template<class R, class I, class P>
        bool operator()(R& r, I i, P p) const
        {
            return std::equal(detail::begin_(r), detail::end_(r), i, p);
        }
    };

    struct search
    {
        template<class R, class R2, class P = void>
        struct result : range_result_iterator<R>
        {};

        template<class R, class R2>
        typename result<R, R2>::type operator()(R& r, R2& r2) const
        {
            return std::search(
                detail::begin_(r)
                , detail::end_(r)
                , detail::begin_(r2)
                , detail::end_(r2)
                );
        }

        template<class R, class R2, class P>
        typename result<R, R2, P>::type operator()(R& r, R2& r2, P p) const
        {
            return std::search(
                detail::begin_(r)
                , detail::end_(r)
                , detail::begin_(r2)
                , detail::end_(r2)
                , p
                );
        }
    };

    struct lower_bound 
    {
        template<class R, class T, class C = void>
        struct result : range_result_iterator<R>
        {};

        template<class R, class T>
        typename result<R, T>::type execute(R& r, T const& val, mpl::true_) const
        {
            return r.lower_bound(val);
        }

        template<class R, class T>
        typename result<R, T>::type execute(R& r, T const& val, mpl::false_) const
        {
            return std::lower_bound(detail::begin_(r), detail::end_(r), val);
        }

        template<class R, class T>
        typename result<R, T>::type operator()(R& r, T const& val) const
        {
            return execute(r, val, has_lower_bound<R>());
        }

        template<class R, class T, class C>
        typename result<R, T>::type operator()(R& r, T const& val, C c) const
        {
            return std::lower_bound(detail::begin_(r), detail::end_(r), val, c);
        }
    };

    struct upper_bound 
    {
        template<class R, class T, class C = void>
        struct result : range_result_iterator<R>
        {};

        template<class R, class T>
        typename result<R, T>::type execute(R& r, T const& val, mpl::true_) const
        {
            return r.upper_bound(val);
        }

        template<class R, class T>
        typename result<R, T>::type execute(R& r, T const& val, mpl::false_) const
        {
            return std::upper_bound(detail::begin_(r), detail::end_(r), val);
        }

        template<class R, class T>
        typename result<R, T>::type operator()(R& r, T const& val) const
        {
            return execute(r, val, has_upper_bound<R>());
        }

        template<class R, class T, class C>
        typename result<R, T>::type operator()(R& r, T const& val, C c) const
        {
            return std::upper_bound(detail::begin_(r), detail::end_(r), val, c);
        }
    };

    struct equal_range 
    {
        template<class R, class T, class C = void>
        struct result
        {
            typedef std::pair<
                typename range_result_iterator<R>::type
                , typename range_result_iterator<R>::type
            > type;
        };

        template<class R, class T>
        typename result<R, T>::type execute(R& r, T const& val, mpl::true_) const
        {
            return r.equal_range(val);
        }

        template<class R, class T>
        typename result<R, T>::type execute(R& r, T const& val, mpl::false_) const
        {
            return std::equal_range(detail::begin_(r), detail::end_(r), val);
        }

        template<class R, class T>
        typename result<R, T>::type operator()(R& r, T const& val) const
        {
            return execute(r, val, has_equal_range<R>());
        }

        template<class R, class T, class C>
        typename result<R, T>::type operator()(R& r, T const& val, C c) const
        {
            return std::equal_range(detail::begin_(r), detail::end_(r), val, c);
        }
    };

    struct mismatch
    {
        template<class R, class I, class P = void>
        struct result
        {
            typedef std::pair<
                typename range_result_iterator<R>::type
                , typename detail::decay_array<I>::type
            > type;
        };

        template<class R, class I>
        typename result<R, I>::type operator()(R& r, I i) const
        {
            return std::mismatch(detail::begin_(r), detail::end_(r), i);
        }

        template<class R, class I, class P>
        typename result<R, I, P>::type operator()(R& r, I i, P p) const
        {
            return std::mismatch(detail::begin_(r), detail::end_(r), i, p);
        }
    };

    struct binary_search 
    {
        template<class R, class T, class C = void>
        struct result
        {
            typedef bool type;
        };

        template<class R, class T>
        bool operator()(R& r, T const& val) const
        {
            return std::binary_search(detail::begin_(r), detail::end_(r), val);
        }

        template<class R, class T, class C>
        bool operator()(R& r, T const& val, C c) const
        {
            return std::binary_search(detail::begin_(r), detail::end_(r), val, c);
        }
    };

    struct includes 
    {
        template<class R1, class R2, class C = void>
        struct result
        {
            typedef bool type;
        };

        template<class R1, class R2>
        bool operator()(R1& r1, R2& r2) const
        {
            return std::includes(
                detail::begin_(r1), detail::end_(r1)
                , detail::begin_(r2), detail::end_(r2)
                );
        }

        template<class R1, class R2, class C>
        bool operator()(R1& r1, R2& r2, C c) const
        {
            return std::includes(
                detail::begin_(r1), detail::end_(r1)
                , detail::begin_(r2), detail::end_(r2)
                , c
                );
        }
    };

    struct min_element
    {
        template<class R, class P = void>
        struct result : range_result_iterator<R>
        {};

        template<class R>
        typename result<R>::type operator()(R& r) const
        {
            return std::min_element(detail::begin_(r), detail::end_(r));
        }
    
        template<class R, class P>
        typename result<R, P>::type operator()(R& r, P p) const
        {
            return std::min_element(detail::begin_(r), detail::end_(r), p);
        }
    };

    struct max_element
    {
        template<class R, class P = void>
        struct result : range_result_iterator<R>
        {};

        template<class R>
        typename result<R>::type operator()(R& r) const
        {
            return std::max_element(detail::begin_(r), detail::end_(r));
        }
    
        template<class R, class P>
        typename result<R, P>::type operator()(R& r, P p) const
        {
            return std::max_element(detail::begin_(r), detail::end_(r), p);
        }
    };

    struct lexicographical_compare
    {
        template<class R1, class R2, class P = void>
        struct result
        {
            typedef bool type;
        };

        template<class R1, class R2>
        typename result<R1, R2>::type operator()(R1& r1, R2& r2) const
        {
            return std::lexicographical_compare(
                detail::begin_(r1), detail::end_(r1)
                , detail::begin_(r2), detail::end_(r2)
                );
        }
    
        template<class R1, class R2, class P>
        typename result<R1, R2>::type operator()(R1& r1, R2& r2, P p) const
        {
            return std::lexicographical_compare(
                detail::begin_(r1), detail::end_(r1)
                , detail::begin_(r2), detail::end_(r2)
                , p
                );
        }
    };

}

function<impl::find> const find = impl::find();
function<impl::find_if> const find_if = impl::find_if();
function<impl::find_end> const find_end = impl::find_end();
function<impl::find_first_of> const find_first_of = impl::find_first_of();
function<impl::adjacent_find> const adjacent_find = impl::adjacent_find();
function<impl::count> const count = impl::count();
function<impl::count_if> const count_if = impl::count_if();
function<impl::distance> const distance = impl::distance();
function<impl::equal> const equal = impl::equal();
function<impl::search> const search = impl::search();
function<impl::lower_bound> const lower_bound = impl::lower_bound();
function<impl::upper_bound> const upper_bound = impl::upper_bound();
function<impl::equal_range> const equal_range = impl::equal_range();
function<impl::mismatch> const mismatch = impl::mismatch();
function<impl::binary_search> const binary_search = impl::binary_search();
function<impl::includes> const includes = impl::includes();
function<impl::min_element> const min_element = impl::min_element();
function<impl::max_element> const max_element = impl::max_element();
function<impl::lexicographical_compare> const lexicographical_compare = impl::lexicographical_compare();

}}

#endif
