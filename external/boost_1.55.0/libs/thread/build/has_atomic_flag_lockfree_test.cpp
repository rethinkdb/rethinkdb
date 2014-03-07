// Copyright (c) 2013, Petr Machata, Red Hat Inc. 
// 
// Use modification and distribution are subject to the boost Software 
// License, Version 1.0.  (See http://www.boost.org/LICENSE_1_0.txt). 
  
#include "../../../boost/atomic.hpp" 
#include "../../../boost/static_assert.hpp" 

int main(int argc, char *argv[]) 
{ 
  BOOST_STATIC_ASSERT(BOOST_ATOMIC_FLAG_LOCK_FREE); 
  return 0; 
} 