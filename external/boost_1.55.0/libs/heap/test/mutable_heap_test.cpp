/*=============================================================================
    Copyright (c) 2010 Tim Blechmann

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <boost/heap/d_ary_heap.hpp>
#include <boost/heap/fibonacci_heap.hpp>
#include <boost/heap/pairing_heap.hpp>
#include <boost/heap/binomial_heap.hpp>
#include <boost/heap/skew_heap.hpp>

using namespace boost::heap;


typedef fibonacci_heap<struct fwd_declared_struct_1>::handle_type handle_type_1;
typedef d_ary_heap<struct fwd_declared_struct_2, arity<4>, mutable_<true> >::handle_type handle_type_2;
typedef pairing_heap<struct fwd_declared_struct_3>::handle_type handle_type_3;
typedef binomial_heap<struct fwd_declared_struct_4>::handle_type handle_type_4;
typedef skew_heap<struct fwd_declared_struct_5, mutable_<true> >::handle_type handle_type_5;

template <typename HeapType>
void run_handle_as_member_test(void)
{
    typedef typename HeapType::value_type value_type;
    HeapType heap;
    value_type f(2);
    typename value_type::handle_type handle = heap.push(f);
    value_type & fInHeap = *handle;
    fInHeap.handle = handle;
}


struct fibonacci_heap_data
{
    typedef fibonacci_heap<fibonacci_heap_data>::handle_type handle_type;

    handle_type handle;
    int i;

    fibonacci_heap_data(int i):i(i) {}

    bool operator<(fibonacci_heap_data const & rhs) const
    {
        return i < rhs.i;
    }
};

BOOST_AUTO_TEST_CASE( fibonacci_heap_handle_as_member )
{
    run_handle_as_member_test<fibonacci_heap<fibonacci_heap_data> >();
}

struct d_heap_data
{
    typedef d_ary_heap<d_heap_data, arity<4>, mutable_<true> >::handle_type handle_type;

    handle_type handle;
    int i;

    d_heap_data(int i):i(i) {}

    bool operator<(d_heap_data const & rhs) const
    {
        return i < rhs.i;
    }
};


BOOST_AUTO_TEST_CASE( d_heap_handle_as_member )
{
    run_handle_as_member_test<d_ary_heap<d_heap_data, arity<4>, mutable_<true> > >();
}

struct pairing_heap_data
{
    typedef pairing_heap<pairing_heap_data>::handle_type handle_type;

    handle_type handle;
    int i;

    pairing_heap_data(int i):i(i) {}

    bool operator<(pairing_heap_data const & rhs) const
    {
        return i < rhs.i;
    }
};


BOOST_AUTO_TEST_CASE( pairing_heap_handle_as_member )
{
    run_handle_as_member_test<pairing_heap<pairing_heap_data> >();
}


struct binomial_heap_data
{
    typedef binomial_heap<binomial_heap_data>::handle_type handle_type;

    handle_type handle;
    int i;

    binomial_heap_data(int i):i(i) {}

    bool operator<(binomial_heap_data const & rhs) const
    {
            return i < rhs.i;
    }
};


BOOST_AUTO_TEST_CASE( binomial_heap_handle_as_member )
{
    run_handle_as_member_test<binomial_heap<binomial_heap_data> >();
}

struct skew_heap_data
{
    typedef skew_heap<skew_heap_data, mutable_<true> >::handle_type handle_type;

    handle_type handle;
    int i;

    skew_heap_data(int i):i(i) {}

    bool operator<(skew_heap_data const & rhs) const
    {
        return i < rhs.i;
    }
};


BOOST_AUTO_TEST_CASE( skew_heap_handle_as_member )
{
    run_handle_as_member_test<skew_heap<skew_heap_data, mutable_<true> > >();
}
