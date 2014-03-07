//  Copyright (C) Joaquin M Lopez Munoz 2004.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_POINTER_TO_MEMBER_TEMPLATE_PARAMETERS
//  TITLE:         pointers to members as template arguments
//  DESCRIPTION:   Non-type template parameters which take pointers
//                 to members, fail to work correctly.


namespace boost_no_pointer_to_member_template_parameters{

struct pair
{
   int x, y;

   pair(int x_,int y_)
      : x(x_), y(y_)
      {}
};

template<int pair::* PtrToPairMember>
struct foo
{
   int bar(pair& p)
   { 
      return p.*PtrToPairMember;
   }
};

int test()
{
   pair p(0,1);
   foo<&pair::x> fx;
   foo<&pair::y> fy;

   if((fx.bar(p) != 0) || (fy.bar(p) != 1))
      return 1;
   return 0;
}

}





