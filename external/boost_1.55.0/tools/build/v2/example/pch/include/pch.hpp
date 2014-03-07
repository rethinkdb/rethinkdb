/* Copyright 2006 Vladimir Prus

   Distributed under the Boost Software License, Version 1.0. (See
   accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)
*/

#ifdef BOOST_BUILD_PCH_ENABLED

#ifdef FOO2
int bar();
#endif

class TestClass {
public:
    TestClass(int, int) {}
};

#endif
