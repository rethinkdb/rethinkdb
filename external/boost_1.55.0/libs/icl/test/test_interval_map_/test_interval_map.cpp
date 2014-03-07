/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#define BOOST_TEST_MODULE icl::interval_map unit test
#include <libs/icl/test/disable_test_warnings.hpp>
#include <string>
#include <boost/mpl/list.hpp>
#include "../unit_test_unwarned.hpp"


// interval instance types
#include "../test_type_lists.hpp"
#include "../test_value_maker.hpp"
#include "../test_functions.hpp"

#include <boost/icl/separate_interval_set.hpp>
#include <boost/icl/split_interval_set.hpp>
#include <boost/icl/interval_map.hpp>
#include <boost/icl/split_interval_map.hpp>

using namespace std;
using namespace boost;
using namespace unit_test;
using namespace boost::icl;

// -----------------------------------------------------------------------------
// test_interval_map_shared are tests that should give identical results for all
// interval_maps: interval_map and split_interval_map.
#include "../test_interval_map_shared.hpp"

#define INTERVAL_MAP interval_map
#include "../test_interval_map_cases.hpp"


