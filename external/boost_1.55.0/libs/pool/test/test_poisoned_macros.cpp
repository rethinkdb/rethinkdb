/* Copyright (C) 2011 John Maddock
* 
* Use, modification and distribution is subject to the 
* Boost Software License, Version 1.0. (See accompanying
* file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
*/

// 
// Verify that if malloc/free are macros that everything still works OK:
//

#include <functional>
#include <new>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <algorithm>
#include <boost/limits.hpp>
#include <iostream>
#include <locale>

#define malloc(x) undefined_poisoned_symbol
#define free(x) undefined_poisoned_symbol

#include <boost/pool/pool.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <boost/pool/singleton_pool.hpp>

template class boost::object_pool<int, boost::default_user_allocator_new_delete>;
template class boost::object_pool<int, boost::default_user_allocator_malloc_free>;

template class boost::pool<boost::default_user_allocator_new_delete>;
template class boost::pool<boost::default_user_allocator_malloc_free>;

template class boost::pool_allocator<int, boost::default_user_allocator_new_delete>;
template class boost::pool_allocator<int, boost::default_user_allocator_malloc_free>;
template class boost::fast_pool_allocator<int, boost::default_user_allocator_new_delete>;
template class boost::fast_pool_allocator<int, boost::default_user_allocator_malloc_free>;

template class boost::simple_segregated_storage<unsigned>;

template class boost::singleton_pool<int, 32>;
