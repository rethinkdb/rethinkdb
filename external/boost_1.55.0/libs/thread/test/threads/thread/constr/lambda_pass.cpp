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

// <boost/thread/thread.hpp>

// class thread

// template <class Clousure> thread(Clousure f);

#include <new>
#include <cstdlib>
#include <cassert>
#include <boost/thread/thread_only.hpp>
#include <boost/detail/lightweight_test.hpp>

#if ! defined BOOST_NO_CXX11_LAMBDAS

unsigned throw_one = 0xFFFF;

#if defined _GLIBCXX_THROW
void* operator new(std::size_t s) _GLIBCXX_THROW (std::bad_alloc)
#elif defined BOOST_MSVC
void* operator new(std::size_t s)
#else
void* operator new(std::size_t s) throw (std::bad_alloc)
#endif
{
  if (throw_one == 0) throw std::bad_alloc();
  --throw_one;
  return std::malloc(s);
}

#if defined BOOST_MSVC
void operator delete(void* p)
#else
void operator delete(void* p) throw ()
#endif
{
  std::free(p);
}

bool f_run = false;

int main()
{
  {
    f_run = false;
    boost::thread t( []() { f_run = true; } );
    t.join();
    BOOST_TEST(f_run == true);
  }
#ifndef BOOST_MSVC
  {
    f_run = false;
    try
    {
      throw_one = 0;
      boost::thread t( []() { f_run = true; } );
      BOOST_TEST(false);
    }
    catch (...)
    {
      throw_one = 0xFFFF;
      BOOST_TEST(!f_run);
    }
  }
#endif

  return boost::report_errors();
}

#else
int main()
{
  return 0;
}
#endif
