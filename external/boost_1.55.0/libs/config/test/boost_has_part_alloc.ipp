//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_HAS_PARTIAL_STD_ALLOCATOR
//  TITLE:         limited std::allocator support
//  DESCRIPTION:   The std lib has at least some kind of stanfard allocator
//                 with allocate/deallocate members and probably not much more.

#include <memory>

namespace boost_has_partial_std_allocator{

//
// test everything except rebind template members:
//

template <class T>
int test_allocator(const T& i)
{
   typedef std::allocator<int> alloc1_t;
   typedef typename alloc1_t::size_type           size_type;
   typedef typename alloc1_t::difference_type     difference_type;
   typedef typename alloc1_t::pointer             pointer;
   typedef typename alloc1_t::const_pointer       const_pointer;
   typedef typename alloc1_t::reference           reference;
   typedef typename alloc1_t::const_reference     const_reference;
   typedef typename alloc1_t::value_type          value_type;

   alloc1_t a1;

   pointer p = a1.allocate(1);
   const_pointer cp = p;
   a1.construct(p,i);
   size_type s = a1.max_size();
   (void)s;
   reference r = *p;
   const_reference cr = *cp;
   if(p != a1.address(r)) return -1;
   if(cp != a1.address(cr)) return -1;
   a1.destroy(p);
   a1.deallocate(p,1);

   return 0;
}


int test()
{
   return test_allocator(0);
}

}






