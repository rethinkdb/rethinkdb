//  (C) Copyright John Maddock 2012

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for more information.

//  MACRO:         BOOST_NO_CXX11_ATOMIC_SMART_PTR
//  TITLE:         C++11 <memory> does not support atomic smart pointer operations
//  DESCRIPTION:   The compiler does not support the C++11 atomic smart pointer features added to <memory>

#include <memory>

namespace boost_no_cxx11_atomic_smart_ptr {

int test()
{
   std::shared_ptr<int> spi(new int), spi2(new int);
   spi = std::static_pointer_cast<int>(spi);

   atomic_is_lock_free(&spi);
   atomic_load(&spi);
   atomic_load_explicit(&spi, std::memory_order_relaxed);
   atomic_store(&spi, spi2);
   atomic_store_explicit(&spi, spi2, std::memory_order_relaxed);
   atomic_exchange(&spi, spi2);
   atomic_compare_exchange_weak(&spi, &spi2, spi);
   atomic_compare_exchange_strong(&spi, &spi2, spi);
   atomic_compare_exchange_weak_explicit(&spi, &spi2, spi, std::memory_order_relaxed, std::memory_order_relaxed);
   atomic_compare_exchange_strong_explicit(&spi, &spi2, spi, std::memory_order_relaxed, std::memory_order_relaxed);

   return 0;
}

}
