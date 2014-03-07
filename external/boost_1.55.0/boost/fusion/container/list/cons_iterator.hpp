/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2005 Eric Niebler

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_CONS_ITERATOR_07172005_0849)
#define FUSION_CONS_ITERATOR_07172005_0849

#include <boost/type_traits/add_const.hpp>
#include <boost/fusion/support/iterator_base.hpp>
#include <boost/fusion/container/list/detail/deref_impl.hpp>
#include <boost/fusion/container/list/detail/next_impl.hpp>
#include <boost/fusion/container/list/detail/value_of_impl.hpp>
#include <boost/fusion/container/list/detail/equal_to_impl.hpp>
#include <boost/fusion/container/list/list_fwd.hpp>

namespace boost { namespace fusion
{
    struct nil_;
    struct cons_iterator_tag;
    struct forward_traversal_tag;

    template <typename Cons>
    struct cons_iterator_identity;

    template <typename Cons = nil_>
    struct cons_iterator : iterator_base<cons_iterator<Cons> >
    {
        typedef cons_iterator_tag fusion_tag;
        typedef forward_traversal_tag category;
        typedef Cons cons_type;
        typedef cons_iterator_identity<
            typename add_const<Cons>::type> 
        identity;

        explicit cons_iterator(cons_type& in_cons)
            : cons(in_cons) {}

        cons_type& cons;

    private:
        // silence MSVC warning C4512: assignment operator could not be generated
        cons_iterator& operator= (cons_iterator const&);
    };

    struct nil_iterator : iterator_base<nil_iterator>
    {
        typedef forward_traversal_tag category;
        typedef cons_iterator_tag fusion_tag;
        typedef nil_ cons_type;
        typedef cons_iterator_identity<
            add_const<nil_>::type> 
        identity;
        nil_iterator() {}
        explicit nil_iterator(nil_ const&) {}
    };

    template <>
    struct cons_iterator<nil_> : nil_iterator 
    {
        cons_iterator() {}
        explicit cons_iterator(nil_ const&) {}
    };

    template <>
    struct cons_iterator<nil_ const> : nil_iterator 
    {
        cons_iterator() {}
        explicit cons_iterator(nil_ const&) {}
    };

    template <>
    struct cons_iterator<list<> > : nil_iterator 
    {
        cons_iterator() {}
        explicit cons_iterator(nil_ const&) {}
    };

    template <>
    struct cons_iterator<list<> const> : nil_iterator 
    {
        cons_iterator() {}
        explicit cons_iterator(nil_ const&) {}
    };
}}

#endif
