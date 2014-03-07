//  (C) Copyright John Maddock 2008. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_ADL_BARRIER
//  TITLE:         Working ADL barriers.
//  DESCRIPTION:   If the compiler correctly handles ADL.

namespace boost_no_adl_barrier{

namespace xxx {
    namespace nested {
        struct aaa {};
    }
    void begin(nested::aaa) {}
}

namespace nnn {
    void begin(xxx::nested::aaa) {}
}

int test()
{
   using namespace nnn;
   xxx::nested::aaa a;
   begin(a); // ambiguous error in msvc-9.0

   return 0;
}

}




