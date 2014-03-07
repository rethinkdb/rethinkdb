//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP
//  TITLE:         Koenig lookup
//  DESCRIPTION:   Compiler does not implement
//                 argument-dependent lookup (also named Koenig lookup); 
//                 see std::3.4.2 [basic.koenig.lookup]


namespace boost_no_argument_dependent_lookup{

namespace foobar{

struct test{};

void call_test(const test&)
{}

}

int test()
{
   foobar::test t;
   call_test(t);
   return 0;
}


}



