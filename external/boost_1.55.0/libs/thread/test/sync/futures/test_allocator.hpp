//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// Copyright (C) 2011 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_THREAD_TEST_ALLOCATOR_HPP
#define BOOST_THREAD_TEST_ALLOCATOR_HPP

#include <cstddef>
#include <boost/type_traits.hpp>
#include <boost/thread/detail/move.hpp>
#include <cstdlib>
#include <new>
#include <climits>

class test_alloc_base
{
public:
    static int count;
public:
    static int throw_after;
};

int test_alloc_base::count = 0;
int test_alloc_base::throw_after = INT_MAX;

template <class T>
class test_allocator
    : public test_alloc_base
{
    int data_;

    template <class U> friend class test_allocator;
public:

    typedef unsigned                                                   size_type;
    typedef int                                                        difference_type;
    typedef T                                                          value_type;
    typedef value_type*                                                pointer;
    typedef const value_type*                                          const_pointer;
    typedef typename boost::add_lvalue_reference<value_type>::type       reference;
    typedef typename boost::add_lvalue_reference<const value_type>::type const_reference;

    template <class U> struct rebind {typedef test_allocator<U> other;};

    test_allocator() throw() : data_(-1) {}
    explicit test_allocator(int i) throw() : data_(i) {}
    test_allocator(const test_allocator& a) throw()
        : data_(a.data_) {}
    template <class U> test_allocator(const test_allocator<U>& a) throw()
        : data_(a.data_) {}
    ~test_allocator() throw() {data_ = 0;}
    pointer address(reference x) const {return &x;}
    const_pointer address(const_reference x) const {return &x;}
    pointer allocate(size_type n, const void* = 0)
        {
            if (count >= throw_after)
                throw std::bad_alloc();
            ++count;
            return (pointer)std::malloc(n * sizeof(T));
        }
    void deallocate(pointer p, size_type)
        {--count; std::free(p);}
    size_type max_size() const throw()
        {return UINT_MAX / sizeof(T);}
    void construct(pointer p, const T& val)
        {::new(p) T(val);}

    void construct(pointer p, BOOST_THREAD_RV_REF(T) val)
        {::new(p) T(boost::move(val));}

    void destroy(pointer p) {p->~T();}

    friend bool operator==(const test_allocator& x, const test_allocator& y)
        {return x.data_ == y.data_;}
    friend bool operator!=(const test_allocator& x, const test_allocator& y)
        {return !(x == y);}
};

template <>
class test_allocator<void>
    : public test_alloc_base
{
    int data_;

    template <class U> friend class test_allocator;
public:

    typedef unsigned                                                   size_type;
    typedef int                                                        difference_type;
    typedef void                                                       value_type;
    typedef value_type*                                                pointer;
    typedef const value_type*                                          const_pointer;

    template <class U> struct rebind {typedef test_allocator<U> other;};

    test_allocator() throw() : data_(-1) {}
    explicit test_allocator(int i) throw() : data_(i) {}
    test_allocator(const test_allocator& a) throw()
        : data_(a.data_) {}
    template <class U> test_allocator(const test_allocator<U>& a) throw()
        : data_(a.data_) {}
    ~test_allocator() throw() {data_ = 0;}

    friend bool operator==(const test_allocator& x, const test_allocator& y)
        {return x.data_ == y.data_;}
    friend bool operator!=(const test_allocator& x, const test_allocator& y)
        {return !(x == y);}
};

template <class T>
class other_allocator
{
    int data_;

    template <class U> friend class other_allocator;

public:
    typedef T value_type;

    other_allocator() : data_(-1) {}
    explicit other_allocator(int i) : data_(i) {}
    template <class U> other_allocator(const other_allocator<U>& a)
        : data_(a.data_) {}
    T* allocate(std::size_t n)
        {return (T*)std::malloc(n * sizeof(T));}
    void deallocate(T* p, std::size_t)
        {std::free(p);}

    other_allocator select_on_container_copy_construction() const
        {return other_allocator(-2);}

    friend bool operator==(const other_allocator& x, const other_allocator& y)
        {return x.data_ == y.data_;}
    friend bool operator!=(const other_allocator& x, const other_allocator& y)
        {return !(x == y);}

    typedef boost::true_type propagate_on_container_copy_assignment;
    typedef boost::true_type propagate_on_container_move_assignment;
    typedef boost::true_type propagate_on_container_swap;

#ifdef BOOST_NO_SFINAE_EXPR
    std::size_t max_size() const
        {return UINT_MAX / sizeof(T);}
#endif  // BOOST_NO_SFINAE_EXPR

};

#endif  // BOOST_THREAD_TEST_ALLOCATOR_HPP
