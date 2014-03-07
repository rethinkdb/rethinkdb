/*-----------------------------------------------------------------------------+    
Copyright (c) 2011-2011: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#define BOOST_TEST_MODULE icl::fix_icl_after_thread unit test
#include <libs/icl/test/disable_test_warnings.hpp>
#include "../unit_test_unwarned.hpp"

// #include <boost/thread.hpp> MEMO: The problem occured when using thread.hpp
#include <boost/bind.hpp>    // but is also triggered from bind.hpp alone
    // while the cause of it is an error in the msvc-7.1 to 10.0 compilers.
    // A minimal example is provided by test case 'cmp_msvc_value_born_error'
#include <boost/icl/interval_map.hpp>
#include <boost/icl/split_interval_map.hpp>
#include <boost/icl/separate_interval_set.hpp>
#include <boost/icl/split_interval_set.hpp>

using namespace std;
using namespace boost;
using namespace unit_test;
using namespace boost::icl;


BOOST_AUTO_TEST_CASE(dummy)
{
    BOOST_CHECK(true);
}

