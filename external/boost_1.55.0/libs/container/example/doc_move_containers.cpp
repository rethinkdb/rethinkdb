//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2009-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/container/detail/config_begin.hpp>
#include <boost/container/detail/workaround.hpp>
//[doc_move_containers
#include <boost/container/vector.hpp>
#include <boost/move/utility.hpp>
#include <cassert>

//Non-copyable class
class non_copyable
{
   BOOST_MOVABLE_BUT_NOT_COPYABLE(non_copyable)

   public:
   non_copyable(){}
   non_copyable(BOOST_RV_REF(non_copyable)) {}
   non_copyable& operator=(BOOST_RV_REF(non_copyable)) { return *this; }
};

int main ()
{
   using namespace boost::container;

   //Store non-copyable objects in a vector
   vector<non_copyable> v;
   non_copyable nc;
   v.push_back(boost::move(nc));
   assert(v.size() == 1);

   //Reserve no longer needs copy-constructible
   v.reserve(100);
   assert(v.capacity() >= 100);

   //This resize overload only needs movable and default constructible
   v.resize(200);
   assert(v.size() == 200);

   //Containers are also movable
   vector<non_copyable> v_other(boost::move(v));
   assert(v_other.size() == 200);
   assert(v.empty());

   return 0;
}
//]
#include <boost/container/detail/config_end.hpp>
