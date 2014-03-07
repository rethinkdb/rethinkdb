//  common_type_test.cpp  ----------------------------------------------------//

//  Copyright 2010 Beman Dawes

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

#define _CRT_SECURE_NO_WARNINGS  // disable VC++ foolishness

#include "test.hpp"
#ifndef TEST_STD 
#include <boost/type_traits/common_type.hpp>
#else
#include <type_traits>
#endif

struct C1 {
    //~ private:
        //~ C1();
};
    
struct C2 {};



typedef tt::common_type<C1, C2>::type AC;

