//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright David Abrahams, Vicente Botet, Ion Gaztanaga 2009.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/move/detail/config_begin.hpp>
#include <boost/move/iterator.hpp>
#include <boost/container/deque.hpp>
#include <boost/container/list.hpp>
#include <boost/container/stable_vector.hpp>
#include "../example/movable.hpp"

template<class Container>
int move_test()
{
   bool use_move_iterator = false;
   bool done = false;
   while(!done){
      //Default construct 10 movable objects
      Container v(10);

      //Test default constructed value
      if(v.begin()->moved()){
         return 1;
      }

      //Move values
      Container v2;
      if(use_move_iterator){
         ::boost::copy_or_move( boost::make_move_iterator(v.begin())
                              , boost::make_move_iterator(v.end())
                              , boost::back_move_inserter(v2));
      }
      else{
         std::copy(v.begin(), v.end(), boost::back_move_inserter(v2));
      }

      //Test values have been moved
      if(!v.begin()->moved()){
         return 1;
      }

      if(v2.size() != 10){
         return 1;
      }

      if(v2.begin()->moved()){
         return 1;
      }
      done = use_move_iterator;
      use_move_iterator = true;
   }
   return 0;
}

int main()
{
   namespace bc = ::boost::container;

   if(move_test< bc::vector<movable> >()){
      return 1;
   }
   if(move_test< bc::list<movable> >()){
      return 1;
   }
   if(move_test< bc::stable_vector<movable> >()){
      return 1;
   }
   return 0;
}

#include <boost/move/detail/config_end.hpp>
