// Copyright (C) 2000 Stephen Cleary
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_SYS_ALLOCATOR_H
#define BOOST_SYS_ALLOCATOR_H

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4100)
#endif

// Symbols: malloc_allocator, new_delete_allocator

#include <cstddef>
#include <cstdlib>
#include <boost/limits.hpp>
#include <new>

template <typename T>
struct malloc_allocator
{
  typedef T * pointer;
  typedef const T * const_pointer;
  typedef T & reference;
  typedef const T & const_reference;
  typedef T value_type;

  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;

  template <typename U>
  struct rebind
  {
    typedef malloc_allocator<U> other;
  };

  static pointer address(reference r) { return &r; }
  static const_pointer address(const_reference r) { return &r; }
  static pointer allocate(const size_type n, const void* = 0)
  {
    const pointer ret = (pointer) std::malloc(n * sizeof(T));
    if (ret == 0)
      throw std::bad_alloc();
    return ret;
  }
  static void deallocate(const pointer p, const size_type)
  { std::free(p); }
  static size_type max_size() { return (std::numeric_limits<size_type>::max)(); }

  bool operator==(const malloc_allocator &) const { return true; }
  bool operator!=(const malloc_allocator &) const { return false; }

  malloc_allocator() { }
  template <typename U>
  malloc_allocator(const malloc_allocator<U> &) { }

  static void construct(const pointer p, const_reference t)
  { new ((void *) p) T(t); }
  static void destroy(const pointer p)
  { p->~T(); }
};

template <typename T>
struct new_delete_allocator
{
  typedef T * pointer;
  typedef const T * const_pointer;
  typedef T & reference;
  typedef const T & const_reference;
  typedef T value_type;

  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;

  template <typename U>
  struct rebind
  {
    typedef new_delete_allocator<U> other;
  };

  static pointer address(reference r) { return &r; }
  static const_pointer address(const_reference r) { return &r; }
  static pointer allocate(const size_type n, const void* = 0)
  { return (pointer) new char[n * sizeof(T)]; }
  static void deallocate(const pointer p, const size_type)
  { delete [] p; }
  static size_type max_size() { return (std::numeric_limits<size_type>::max)(); }

  bool operator==(const new_delete_allocator &) const { return true; }
  bool operator!=(const new_delete_allocator &) const { return false; }

  new_delete_allocator() { }
  template <typename U>
  new_delete_allocator(const new_delete_allocator<U> &) { }

  static void construct(const pointer p, const_reference t)
  { new ((void *) p) T(t); }
  static void destroy(const pointer p)
  { p->~T(); }
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
