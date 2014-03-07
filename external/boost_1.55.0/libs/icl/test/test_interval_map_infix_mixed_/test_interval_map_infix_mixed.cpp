/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#define BOOST_TEST_MODULE icl::interval_map_infix_mixed unit test
#include <libs/icl/test/disable_test_warnings.hpp>
#include <string>
#include <boost/mpl/list.hpp>
#include "../unit_test_unwarned.hpp"


// interval instance types
#include "../test_type_lists.hpp"
#include "../test_value_maker.hpp"

#include <boost/icl/interval_set.hpp>
#include <boost/icl/separate_interval_set.hpp>
#include <boost/icl/split_interval_set.hpp>
#include <boost/icl/interval_map.hpp>
#include <boost/icl/split_interval_map.hpp>

using namespace std;
using namespace boost;
using namespace unit_test;
using namespace boost::icl;


BOOST_AUTO_TEST_CASE_TEMPLATE(test_icl_interval_map_mixed_infix_plus_overload_4_bicremental_types, T, bicremental_types)
{
    typedef int U;
    typedef interval_map<T,U>  IntervalMapT;
    interval_map<T,U>          join_a;
    split_interval_map<T,U>    split_a;

    join_a .add(CDv(1,3,1)).add(IDv(8,9,1)).add(IIv(6,11,3));
    split_a.add(IDv(0,9,2)).add(IIv(3,6,1)).add(IDv(5,7,1));

    BOOST_CHECK_EQUAL(split_a + join_a, join_a + split_a);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_icl_interval_map_mixed_infix_pipe_overload_4_bicremental_types, T, bicremental_types)
{
    typedef int U;
    typedef interval_map<T,U>  IntervalMapT;
    interval_map<T,U>          join_a;
    split_interval_map<T,U>    split_a;

    join_a .add(CDv(1,3,1)).add(IDv(8,9,1)).add(IIv(6,11,3));
    split_a.add(IDv(0,9,2)).add(IIv(3,6,1)).add(IDv(5,7,1));

    BOOST_CHECK_EQUAL(split_a | join_a, join_a | split_a);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_icl_interval_map_mixed_infix_minus_overload_4_bicremental_types, T, bicremental_types)
{
    typedef int U;
    typedef interval_map<T,U>  IntervalMapT;
    interval_map<T,U>          join_a, join_b;
    split_interval_map<T,U>    split_a, split_b;

    join_a .add(CDv(1,3,1)).add(IDv(8,9,1)).add(IIv(6,11,3));
    split_a.add(IDv(0,9,2)).add(IIv(3,6,1)).add(IDv(5,7,1));

    join_b .add(CDv(1,3,1)).add(IIv(6,11,3));
    split_b.add(IDv(0,9,2)).add(IIv(3,6,1));

    BOOST_CHECK_EQUAL(split_a - join_a, (split_b = split_a) -= join_a);
    BOOST_CHECK_EQUAL(split_a - join_a, split_b);

    BOOST_CHECK_EQUAL(join_a - split_a, (join_b = join_a) -= split_a);
    BOOST_CHECK_EQUAL(join_a - split_a, join_b);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_icl_interval_map_mixed_infix_et_overload_4_bicremental_types, T, bicremental_types)
{
    typedef int U;
    typedef interval_map<T,U>  IntervalMapT;
    interval_map<T,U>          join_a, join_b;
    split_interval_map<T,U>    split_a, split_b;

    join_a .add(CDv(1,3,1)).add(IDv(8,9,1)).add(IIv(6,11,3));
    split_a.add(IDv(0,9,2)).add(IIv(3,6,1)).add(IDv(5,7,1));

    BOOST_CHECK_EQUAL(split_a & join_a, join_a & split_a);
    BOOST_CHECK_EQUAL(split_a & join_a, (split_b = split_a) &= join_a);
    BOOST_CHECK_EQUAL(split_a & join_a, split_b);

    BOOST_CHECK_EQUAL(join_a & split_a, (split_b = split_a) &= join_a);
    BOOST_CHECK_EQUAL(join_a & split_a, split_b);
}

