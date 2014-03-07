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

//[construct_forward_example
#include <boost/move/utility.hpp>
#include <iostream>

class copyable_only_tester
{
   public:
   copyable_only_tester()
   {  std::cout << "copyable_only_tester()" << std::endl;   }

   copyable_only_tester(const copyable_only_tester&)
   {  std::cout << "copyable_only_tester(const copyable_only_tester&)" << std::endl;   }

   copyable_only_tester(int)
   {  std::cout << "copyable_only_tester(int)" << std::endl;   }

   copyable_only_tester(int, double)
   {  std::cout << "copyable_only_tester(int, double)" << std::endl;   }
};

class copyable_movable_tester
{
   // move semantics
   BOOST_COPYABLE_AND_MOVABLE(copyable_movable_tester)
   public:

   copyable_movable_tester()
   {  std::cout << "copyable_movable_tester()" << std::endl;   }

   copyable_movable_tester(int)
   {  std::cout << "copyable_movable_tester(int)" << std::endl;   }

   copyable_movable_tester(BOOST_RV_REF(copyable_movable_tester))
   {  std::cout << "copyable_movable_tester(BOOST_RV_REF(copyable_movable_tester))" << std::endl;   }

   copyable_movable_tester(const copyable_movable_tester &)
   {  std::cout << "copyable_movable_tester(const copyable_movable_tester &)" << std::endl;   }

   copyable_movable_tester(BOOST_RV_REF(copyable_movable_tester), BOOST_RV_REF(copyable_movable_tester))
   {  std::cout << "copyable_movable_tester(BOOST_RV_REF(copyable_movable_tester), BOOST_RV_REF(copyable_movable_tester))" << std::endl;   }

   copyable_movable_tester &operator=(BOOST_RV_REF(copyable_movable_tester))
   {  std::cout << "copyable_movable_tester & operator=(BOOST_RV_REF(copyable_movable_tester))" << std::endl; 
      return *this;  }

   copyable_movable_tester &operator=(BOOST_COPY_ASSIGN_REF(copyable_movable_tester))
   {  std::cout << "copyable_movable_tester & operator=(BOOST_COPY_ASSIGN_REF(copyable_movable_tester))" << std::endl;
      return *this;  }
};

//1 argument
template<class MaybeMovable, class MaybeRv>
void function_construct(BOOST_FWD_REF(MaybeRv) x)
{  MaybeMovable m(boost::forward<MaybeRv>(x));   }

//2 argument
template<class MaybeMovable, class MaybeRv, class MaybeRv2>
void function_construct(BOOST_FWD_REF(MaybeRv) x, BOOST_FWD_REF(MaybeRv2) x2)
{  MaybeMovable m(boost::forward<MaybeRv>(x), boost::forward<MaybeRv2>(x2));  }

int main()
{
   copyable_movable_tester m;
   //move constructor
   function_construct<copyable_movable_tester>(boost::move(m));
   //copy constructor
   function_construct<copyable_movable_tester>(copyable_movable_tester());
   //two rvalue constructor
   function_construct<copyable_movable_tester>(boost::move(m), boost::move(m));

   copyable_only_tester nm;
   //copy constructor (copyable_only_tester has no move ctor.)
   function_construct<copyable_only_tester>(boost::move(nm));
   //copy constructor
   function_construct<copyable_only_tester>(nm);
   //int constructor
   function_construct<copyable_only_tester>(int(0));
   //int, double constructor
   function_construct<copyable_only_tester>(int(0), double(0.0));

   //Output is:
   //copyable_movable_tester()
   //copyable_movable_tester(BOOST_RV_REF(copyable_movable_tester))
   //copyable_movable_tester()
   //copyable_movable_tester(const copyable_movable_tester &)
   //copyable_movable_tester(BOOST_RV_REF(copyable_movable_tester), BOOST_RV_REF(copyable_movable_tester))
   //copyable_only_tester()
   //copyable_only_tester(const copyable_only_tester&)
   //copyable_only_tester(const copyable_only_tester&)
   //copyable_only_tester(int)
   //copyable_only_tester(int, double)
   return 0;
}
//]

#include <boost/move/detail/config_end.hpp>
