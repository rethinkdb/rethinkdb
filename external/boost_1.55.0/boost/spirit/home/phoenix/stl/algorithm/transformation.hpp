// Copyright 2005 Daniel Wallin. 
// Copyright 2005 Joel de Guzman.
// Copyright 2005 Dan Marsden. 
//
// Use, modification and distribution is subject to the Boost Software 
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Modeled after range_ex, Copyright 2004 Eric Niebler

#ifndef PHOENIX_ALGORITHM_TRANSFORMATION_HPP
#define PHOENIX_ALGORITHM_TRANSFORMATION_HPP

#include <algorithm>
#include <numeric>

#include <boost/spirit/home/phoenix/stl/algorithm/detail/has_sort.hpp>
#include <boost/spirit/home/phoenix/stl/algorithm/detail/has_remove.hpp>
#include <boost/spirit/home/phoenix/stl/algorithm/detail/has_remove_if.hpp>
#include <boost/spirit/home/phoenix/stl/algorithm/detail/has_unique.hpp>
#include <boost/spirit/home/phoenix/stl/algorithm/detail/has_reverse.hpp>
#include <boost/spirit/home/phoenix/stl/algorithm/detail/has_sort.hpp>

#include <boost/spirit/home/phoenix/stl/algorithm/detail/begin.hpp>
#include <boost/spirit/home/phoenix/stl/algorithm/detail/end.hpp>
#include <boost/spirit/home/phoenix/stl/algorithm/detail/decay_array.hpp>

#include <boost/spirit/home/phoenix/function/function.hpp>

#include <boost/range/result_iterator.hpp>
#include <boost/range/difference_type.hpp>

#include <boost/mpl/if.hpp>

#include <boost/type_traits/is_void.hpp>

namespace boost { namespace phoenix { namespace impl
{
    struct swap
    {
        template <class A, class B>
        struct result
        {
            typedef void type;
        };

        template <class A, class B>
        void operator()(A& a, B& b) const
        {
            using std::swap;
            swap(a, b);
        }
    };

    struct copy
    {
        template<class R, class I>
        struct result
            : detail::decay_array<I>
        {};

        template<class R, class I>
        typename result<R,I>::type
        operator()(R& r, I i) const
        {
            return std::copy(detail::begin_(r), detail::end_(r), i);
        }
    };

    struct copy_backward
    {
        template<class R, class I>
        struct result
        {
            typedef I type;
        };

        template<class R, class I>
        I operator()(R& r, I i) const
        {
            return std::copy_backward(detail::begin_(r), detail::end_(r), i);
        }
    };

    struct transform
    {
        template<class R, class OutorI1, class ForOut, class BinF = void>
        struct result
            : detail::decay_array<
            typename mpl::if_<is_void<BinF>, OutorI1, ForOut>::type>
        {
        };

        template<class R, class O, class F>
        typename result<R,O,F>::type
        operator()(R& r, O o, F f) const
        {
            return std::transform(detail::begin_(r), detail::end_(r), o, f);
        }

        template<class R, class I, class O, class F>
        typename result<R,I,O,F>::type
        operator()(R& r, I i, O o, F f) const
        {
            return std::transform(detail::begin_(r), detail::end_(r), i, o, f);
        }
    };

    struct replace
    {
        template<class R, class T, class T2>
        struct result
        {
            typedef void type;
        };

        template<class R, class T>
        void operator()(R& r, T const& what, T const& with) const
        {
            std::replace(detail::begin_(r), detail::end_(r), what, with);
        }
    };

    struct replace_if
    {
        template<class R, class P, class T>
        struct result
        {
            typedef void type;
        };

        template<class R, class P, class T>
        void operator()(R& r, P p, T const& with) const
        {
            std::replace_if(detail::begin_(r), detail::end_(r), p, with);
        }
    };

    struct replace_copy
    {
        template<class R, class O, class T, class T2>
        struct result
            : detail::decay_array<O>
        {};

        template<class R, class O, class T>
        typename result<R,O,T,T>::type 
        operator()(R& r, O o, T const& what, T const& with) const
        {
            return std::replace_copy(detail::begin_(r), detail::end_(r), o, what, with);
        }
    };

    struct replace_copy_if
    {
        template<class R, class O, class P, class T>
        struct result
            : detail::decay_array<O>
        {};

        template<class R, class O, class P, class T>
        typename result<R,O,P,T>::type
        operator()(R& r, O o, P p, T const& with) const
        {
            return std::replace_copy_if(detail::begin_(r), detail::end_(r), o, p, with);
        }
    };

    struct fill
    {
        template<class R, class T>
        struct result
        {
            typedef void type;
        };

        template<class R, class T>
        void operator()(R& r, T const& x) const
        {
            std::fill(detail::begin_(r), detail::end_(r), x);
        }
    };

    struct fill_n
    {
        template<class R, class N, class T>
        struct result
        {
            typedef void type;
        };

        template<class R, class N, class T>
        void operator()(R& r, N n, T const& x) const
        {
            std::fill_n(detail::begin_(r), n, x);
        }
    };

    struct generate
    {
        template<class R, class G>
        struct result
        {
            typedef void type;
        };

        template<class R, class G>
        void operator()(R& r, G g) const
        {
            std::generate(detail::begin_(r), detail::end_(r), g);
        }
    };

    struct generate_n
    {
        template<class R, class N, class G>
        struct result
        {
            typedef void type;
        };

        template<class R, class N, class G>
        void operator()(R& r, N n, G g) const
        {
            std::generate_n(detail::begin_(r), n, g);
        }
    };

    struct remove
    {
        template<class R, class T>
        struct result : range_result_iterator<R>
        {
        };

        template<class R, class T>
        typename result<R, T>::type execute(R& r, T const& x, mpl::true_) const
        {
            r.remove(x);
            return detail::end_(r);
        }

        template<class R, class T>
        typename result<R, T>::type execute(R& r, T const& x, mpl::false_) const
        {
            return std::remove(detail::begin_(r), detail::end_(r), x);
        }

        template<class R, class T>
        typename result<R, T>::type operator()(R& r, T const& x) const
        {
            return execute(r, x, has_remove<R>());
        }
    };

    struct remove_if
    {
        template<class R, class P>
        struct result : range_result_iterator<R>
        {
        };

        template<class R, class P>
        typename result<R, P>::type execute(R& r, P p, mpl::true_) const
        {
            r.remove_if(p);
            return detail::end_(r);
        }

        template<class R, class P>
        typename result<R, P>::type execute(R& r, P p, mpl::false_) const
        {
            return std::remove_if(detail::begin_(r), detail::end_(r), p);
        }

        template<class R, class P>
        typename result<R, P>::type operator()(R& r, P p) const
        {
            return execute(r, p, has_remove_if<R>());
        }
    };

    struct remove_copy
    {
        template<class R, class O, class T>
        struct result
            : detail::decay_array<O>
        {};

        template<class R, class O, class T>
        typename result<R,O,T>::type
        operator()(R& r, O o, T const& x) const
        {
            return std::remove_copy(detail::begin_(r), detail::end_(r), o, x);
        }
    };

    struct remove_copy_if
    {
        template<class R, class O, class P>
        struct result
            : detail::decay_array<O>
        {};

        template<class R, class O, class P>
        typename result<R,O,P>::type
        operator()(R& r, O o, P p) const
        {
            return std::remove_copy_if(detail::begin_(r), detail::end_(r), o, p);
        }
    };

    struct unique
    {
        template<class R, class P = void>
        struct result : range_result_iterator<R>
        {
        };

        template<class R>
        typename result<R>::type execute(R& r, mpl::true_) const
        {
            r.unique();
            return detail::end_(r);
        }

        template<class R>
        typename result<R>::type execute(R& r, mpl::false_) const
        {
            return std::unique(detail::begin_(r), detail::end_(r));
        }

        template<class R>
        typename result<R>::type operator()(R& r) const
        {
            return execute(r, has_unique<R>());
        }


        template<class R, class P>
        typename result<R>::type execute(R& r, P p, mpl::true_) const
        {
            r.unique(p);
            return detail::end_(r);
        }

        template<class R, class P>
        typename result<R, P>::type execute(R& r, P p, mpl::false_) const
        {
            return std::unique(detail::begin_(r), detail::end_(r), p);
        }

        template<class R, class P>
        typename result<R, P>::type operator()(R& r, P p) const
        {
            return execute(r, p, has_unique<R>());
        }
    };

    struct unique_copy
    {
        template<class R, class O, class P = void>
        struct result
            : detail::decay_array<O>
        {};

        template<class R, class O>
        typename result<R, O>::type operator()(R& r, O o) const
        {
            return std::unique_copy(
                detail::begin_(r)
                , detail::end_(r)
                , o
                );
        }

        template<class R, class O, class P>
        typename result<R, O, P>::type operator()(R& r, O o, P p) const
        {
            return std::unique_copy(
                detail::begin_(r)
                , detail::end_(r)
                , o
                , p
                );
        }
    };

    struct reverse
    {
        template<class R>
        struct result
        {
            typedef void type;
        };

        template<class R>
        void execute(R& r, mpl::true_) const
        {
            r.reverse();
        }

        template<class R>
        void execute(R& r, mpl::false_) const
        {
            std::reverse(detail::begin_(r), detail::end_(r));
        }

        template<class R>
        void operator()(R& r) const
        {
            execute(r, has_reverse<R>());
        }
    };

    struct reverse_copy
    {
        template<class R, class O>
        struct result
            : detail::decay_array<O>
        {};

        template<class R, class O>
        typename result<R, O>::type operator()(R& r, O o) const
        {
            return std::reverse_copy(
                detail::begin_(r)
                , detail::end_(r)
                , o
                );
        }
    };

    struct rotate
    {
        template<class R, class M>
        struct result
        {
            typedef void type;
        };

        template<class R, class M>
        void operator()(R& r, M m) const
        {
            std::rotate(
                detail::begin_(r)
                , m
                , detail::end_(r)
                );
        }
    };

    struct rotate_copy
    {
        template<class R, class M, class O>
        struct result
            : detail::decay_array<O>
        {};

        template<class R, class M, class O>
        typename result<R, M, O>::type operator()(R& r, M m, O o) const
        {
            return std::rotate_copy(
                detail::begin_(r)
                , m
                , detail::end_(r)
                , o
                );
        }
    };

    struct random_shuffle
    {
        template<class R, class G = void>
        struct result
        {
            typedef void type;
        };

        template<class R>
        void operator()(R& r) const
        {
            return std::random_shuffle(detail::begin_(r), detail::end_(r));
        }

        template<class R, class G>
        void operator()(R& r, G g) const
        {
            return std::random_shuffle(detail::begin_(r), detail::end_(r), g);
        }
    };

    struct partition
    {
        template<class R, class P>
        struct result : range_result_iterator<R>
        {};

        template<class R, class P>
        typename result<R, P>::type operator()(R& r, P p) const
        {
            return std::partition(detail::begin_(r), detail::end_(r), p);
        }
    };

    struct stable_partition
    {
        template<class R, class P>
        struct result : range_result_iterator<R>
        {};

        template<class R, class P>
        typename result<R, P>::type operator()(R& r, P p) const
        {
            return std::stable_partition(detail::begin_(r), detail::end_(r), p);
        }
    };

    struct sort
    {
        template<class R, class C = void>
        struct result
        {
            typedef void type;
        };

        template<class R>
        void execute(R& r, mpl::true_) const
        {
            r.sort();
        }

        template<class R>
        void execute(R& r, mpl::false_) const
        {
            std::sort(detail::begin_(r), detail::end_(r));
        }

        template<class R>
        void operator()(R& r) const
        {
            execute(r, has_sort<R>());
        }

        template<class R, class C>
        void execute(R& r, C c, mpl::true_) const
        {
            r.sort(c);
        }

        template<class R, class C>
        void execute(R& r, C c, mpl::false_) const
        {
            std::sort(detail::begin_(r), detail::end_(r), c);
        }

        template<class R, class C>
        void operator()(R& r, C c) const
        {
            execute(r, c, has_sort<R>());
        }
    };

    struct stable_sort
    {
        template<class R, class C = void>
        struct result
        {
            typedef void type;
        };

        template<class R>
        void operator()(R& r) const
        {
            std::stable_sort(detail::begin_(r), detail::end_(r));
        }

        template<class R, class C>
        void operator()(R& r, C c) const
        {
            std::stable_sort(detail::begin_(r), detail::end_(r), c);
        }
    };

    struct partial_sort
    {
        template<class R, class M, class C = void>
        struct result
        {
            typedef void type;
        };

        template<class R, class M>
        void operator()(R& r, M m) const
        {
            std::partial_sort(detail::begin_(r), m, detail::end_(r));
        }

        template<class R, class M, class C>
        void operator()(R& r, M m, C c) const
        {
            std::partial_sort(detail::begin_(r), m, detail::end_(r), c);
        }
    };

    struct partial_sort_copy
    {
        template<class R1, class R2, class C = void>
        struct result : range_result_iterator<R2>
        {};

        template<class R1, class R2>
        typename result<R1, R2>::type operator()(R1& r1, R2& r2) const
        {
            return std::partial_sort_copy(
                detail::begin_(r1), detail::end_(r1)
                , detail::begin_(r2), detail::end_(r2)
                );
        }

        template<class R1, class R2, class C>
        typename result<R1, R2>::type operator()(R1& r1, R2& r2, C c) const
        {
            return std::partial_sort_copy(
                detail::begin_(r1), detail::end_(r1)
                , detail::begin_(r2), detail::end_(r2)
                , c
                );
        }
    };

    struct nth_element
    {
        template<class R, class N, class C = void>
        struct result
        {
            typedef void type;
        };

        template<class R, class N>
        void operator()(R& r, N n) const
        {
            return std::nth_element(detail::begin_(r), n, detail::end_(r));
        }

        template<class R, class N, class C>
        void operator()(R& r, N n, C c) const
        {
            return std::nth_element(detail::begin_(r), n, detail::end_(r), c);
        }
    };

    struct merge 
    {
        template<class R1, class R2, class O, class C = void>
        struct result
            : detail::decay_array<O>
        {};

        template<class R1, class R2, class O>
        typename result<R1, R2, O>::type operator()(R1& r1, R2& r2, O o) const
        {
            return std::merge(
                detail::begin_(r1), detail::end_(r1)
                , detail::begin_(r2), detail::end_(r2)
                , o
                );
        }

        template<class R1, class R2, class O, class C>
        typename result<R1, R2, O, C>::type operator()(R1& r1, R2& r2, O o, C c) const
        {
            return std::merge(
                detail::begin_(r1), detail::end_(r1)
                , detail::begin_(r2), detail::end_(r2)
                , o
                , c
                );
        }
    };

    struct inplace_merge 
    {
        template<class R, class M, class C = void>
        struct result
        {
            typedef void type;
        };

        template<class R, class M>
        void operator()(R& r, M m) const
        {
            return std::inplace_merge(detail::begin_(r), m, detail::end_(r));
        }

        template<class R, class M, class C>
        void operator()(R& r, M m, C c) const
        {
            return std::inplace_merge(detail::begin_(r), m, detail::end_(r), c);
        }
    };

    struct next_permutation
    {
        template<class R, class C = void>
        struct result
        {
            typedef bool type;
        };

        template<class R>
        bool operator()(R& r) const
        {
            return std::next_permutation(detail::begin_(r), detail::end_(r));
        }
    
        template<class R, class C>
        bool operator()(R& r, C c) const
        {
            return std::next_permutation(detail::begin_(r), detail::end_(r), c);
        }
    };

    struct prev_permutation
    {
        template<class R, class C = void>
        struct result
        {
            typedef bool type;
        };

        template<class R>
        bool operator()(R& r) const
        {
            return std::prev_permutation(detail::begin_(r), detail::end_(r));
        }
    
        template<class R, class C>
        bool operator()(R& r, C c) const
        {
            return std::prev_permutation(detail::begin_(r), detail::end_(r), c);
        }
    };


    struct inner_product
    {
        template<class R, class I, class T, class C1 = void, class C2 = void>
        struct result
        {
            typedef T type;
        };

        template<class R, class I, class T>
        typename result<R,I,T>::type
        operator()(R& r, I i, T t) const
        {
            return std::inner_product(
                detail::begin_(r), detail::end_(r), i, t);
        }

        template<class R, class I, class T, class C1, class C2>
        typename result<R,I,T,C1,C2>::type
        operator()(R& r, I i, T t, C1 c1, C2 c2) const
        {
            return std::inner_product(
                detail::begin_(r), detail::end_(r), i, 
                t, c1, c2);
        }
    };

    struct partial_sum
    {
        template<class R, class I, class C = void>
        struct result
            : detail::decay_array<I>
        {};

        template<class R, class I>
        typename result<R,I>::type
        operator()(R& r, I i) const
        {
            return std::partial_sum(
                detail::begin_(r), detail::end_(r), i);
        }

        template<class R, class I, class C>
        typename result<R,I,C>::type
        operator()(R& r, I i, C c) const
        {
            return std::partial_sum(
                detail::begin_(r), detail::end_(r), i, c);
        }
    };

    struct adjacent_difference
    {
        template<class R, class I, class C = void>
        struct result
            : detail::decay_array<I>
        {};

        template<class R, class I>
        typename result<R,I>::type
        operator()(R& r, I i) const
        {
            return std::adjacent_difference(
                detail::begin_(r), detail::end_(r), i);
        }

        template<class R, class I, class C>
        typename result<R,I,C>::type
        operator()(R& r, I i, C c) const
        {
            return std::adjacent_difference(
                detail::begin_(r), detail::end_(r), i, c);
        }    
    };

    struct push_heap
    {
        template<class R, class C = void>
        struct result
        {
            typedef void type;
        };

        template<class R>
        void operator()(R& r) const
        {
            std::push_heap(detail::begin_(r), detail::end_(r));
        }

        template<class R, class C>
        void operator()(R& r, C c) const
        {
            std::push_heap(detail::begin_(r), detail::end_(r), c);
        }
    };

    struct pop_heap
    {
        template<class R, class C = void>
        struct result
        {
            typedef void type;
        };

        template<class R>
        void operator()(R& r) const
        {
            std::pop_heap(detail::begin_(r), detail::end_(r));
        }

        template<class R, class C>
        void operator()(R& r, C c) const
        {
            std::pop_heap(detail::begin_(r), detail::end_(r), c);
        }
    };

    struct make_heap
    {
        template<class R, class C = void>
        struct result
        {
            typedef void type;
        };

        template<class R>
        void operator()(R& r) const
        {
            std::make_heap(detail::begin_(r), detail::end_(r));
        }

        template<class R, class C>
        void operator()(R& r, C c) const
        {
            std::make_heap(detail::begin_(r), detail::end_(r), c);
        }
    };

    struct sort_heap
    {
        template<class R, class C = void>
        struct result
        {
            typedef void type;
        };

        template<class R>
        void operator()(R& r) const
        {
            std::sort_heap(detail::begin_(r), detail::end_(r));
        }

        template<class R, class C>
        void operator()(R& r, C c) const
        {
            std::sort_heap(detail::begin_(r), detail::end_(r), c);
        }
    };

    struct set_union
    {
        template<class R1, class R2, class O, class C = void>
        struct result
            : detail::decay_array<O>
        {};

        template<class R1, class R2, class O>
        typename result<R1, R2, O>::type operator()(R1& r1, R2& r2, O o) const
        {
            return std::set_union(
                detail::begin_(r1), detail::end_(r1)
                , detail::begin_(r2), detail::end_(r2)
                , o
                );
        }

        template<class R1, class R2, class O, class C>
        typename result<R1, R2, O, C>::type operator()(R1& r1, R2& r2, O o, C c) const
        {
            return std::set_union(
                detail::begin_(r1), detail::end_(r1)
                , detail::begin_(r2), detail::end_(r2)
                , o
                , c
                );
        }
    };

    struct set_intersection
    {
        template<class R1, class R2, class O, class C = void>
        struct result
            : detail::decay_array<O>
        {};

        template<class R1, class R2, class O>
        typename result<R1, R2, O>::type operator()(R1& r1, R2& r2, O o) const
        {
            return std::set_intersection(
                detail::begin_(r1), detail::end_(r1)
                , detail::begin_(r2), detail::end_(r2)
                , o
                );
        }

        template<class R1, class R2, class O, class C>
        typename result<R1, R2, O, C>::type operator()(R1& r1, R2& r2, O o, C c) const
        {
            return std::set_intersection(
                detail::begin_(r1), detail::end_(r1)
                , detail::begin_(r2), detail::end_(r2)
                , o
                , c
                );
        }
    };

    struct set_difference
    {
        template<class R1, class R2, class O, class C = void>
        struct result
            : detail::decay_array<O>
        {};

        template<class R1, class R2, class O>
        typename result<R1, R2, O>::type operator()(R1& r1, R2& r2, O o) const
        {
            return std::set_difference(
                detail::begin_(r1), detail::end_(r1)
                , detail::begin_(r2), detail::end_(r2)
                , o
                );
        }

        template<class R1, class R2, class O, class C>
        typename result<R1, R2, O, C>::type operator()(R1& r1, R2& r2, O o, C c) const
        {
            return std::set_difference(
                detail::begin_(r1), detail::end_(r1)
                , detail::begin_(r2), detail::end_(r2)
                , o
                , c
                );
        }
    };

    struct set_symmetric_difference
    {
        template<class R1, class R2, class O, class C = void>
        struct result
            : detail::decay_array<O>
        {};

        template<class R1, class R2, class O>
        typename result<R1, R2, O>::type operator()(R1& r1, R2& r2, O o) const
        {
            return std::set_symmetric_difference(
                detail::begin_(r1), detail::end_(r1)
                , detail::begin_(r2), detail::end_(r2)
                , o
                );
        }

        template<class R1, class R2, class O, class C>
        typename result<R1, R2, O, C>::type operator()(R1& r1, R2& r2, O o, C c) const
        {
            return std::set_symmetric_difference(
                detail::begin_(r1), detail::end_(r1)
                , detail::begin_(r2), detail::end_(r2)
                , o
                , c
                );
        }
    };

}}} // boost::phoenix::impl

namespace boost { namespace phoenix
{
    function<impl::swap> const swap = impl::swap();
    function<impl::copy> const copy = impl::copy();
    function<impl::copy_backward> const copy_backward = impl::copy_backward();
    function<impl::transform> const transform = impl::transform();
    function<impl::replace> const replace = impl::replace();
    function<impl::replace_if> const replace_if = impl::replace_if();
    function<impl::replace_copy> const replace_copy = impl::replace_copy();
    function<impl::replace_copy_if> const replace_copy_if = impl::replace_copy_if();
    function<impl::fill> const fill = impl::fill();
    function<impl::fill_n> const fill_n = impl::fill_n();
    function<impl::generate> const generate = impl::generate();
    function<impl::generate_n> const generate_n = impl::generate_n();
    function<impl::remove> const remove = impl::remove();
    function<impl::remove_if> const remove_if = impl::remove_if();
    function<impl::remove_copy> const remove_copy = impl::remove_copy();
    function<impl::remove_copy_if> const remove_copy_if = impl::remove_copy_if();
    function<impl::unique> const unique = impl::unique();
    function<impl::unique_copy> const unique_copy = impl::unique_copy();
    function<impl::reverse> const reverse = impl::reverse();
    function<impl::reverse_copy> const reverse_copy = impl::reverse_copy();
    function<impl::rotate> const rotate = impl::rotate();
    function<impl::rotate_copy> const rotate_copy = impl::rotate_copy();
    function<impl::random_shuffle> const random_shuffle = impl::random_shuffle();
    function<impl::partition> const partition = impl::partition();
    function<impl::stable_partition> const stable_partition = impl::stable_partition();
    function<impl::sort> const sort = impl::sort();
    function<impl::stable_sort> const stable_sort = impl::stable_sort();
    function<impl::partial_sort> const partial_sort = impl::partial_sort();
    function<impl::partial_sort_copy> const partial_sort_copy = impl::partial_sort_copy();
    function<impl::nth_element> const nth_element = impl::nth_element();
    function<impl::merge> const merge = impl::merge();
    function<impl::inplace_merge> const inplace_merge = impl::inplace_merge();
    function<impl::next_permutation> const next_permutation = impl::next_permutation();
    function<impl::prev_permutation> const prev_permutation = impl::prev_permutation();
    function<impl::inner_product> const inner_product = impl::inner_product();
    function<impl::partial_sum> const partial_sum = impl::partial_sum();
    function<impl::adjacent_difference> const adjacent_difference = impl::adjacent_difference();
    function<impl::push_heap> const push_heap = impl::push_heap();
    function<impl::pop_heap> const pop_heap = impl::pop_heap();
    function<impl::make_heap> const make_heap = impl::make_heap();
    function<impl::sort_heap> const sort_heap = impl::sort_heap();
    function<impl::set_union> const set_union = impl::set_union();
    function<impl::set_intersection> const set_intersection = impl::set_intersection();
    function<impl::set_difference> const set_difference = impl::set_difference();
    function<impl::set_symmetric_difference> const set_symmetric_difference = impl::set_symmetric_difference();
}}

#endif
