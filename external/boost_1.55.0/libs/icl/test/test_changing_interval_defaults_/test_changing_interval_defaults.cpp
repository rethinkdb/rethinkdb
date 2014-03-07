/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#define BOOST_TEST_MODULE icl::test_doc_code unit test
#include <libs/icl/test/disable_test_warnings.hpp>

#include <limits>
#include <complex>


#include <string>
#include <vector>
#include <boost/mpl/list.hpp>
#include "../unit_test_unwarned.hpp"


// interval instance types
#include "../test_type_lists.hpp"
#include "../test_value_maker.hpp"

#include <boost/type_traits/is_same.hpp>


#define BOOST_ICL_USE_STATIC_BOUNDED_INTERVALS
#define BOOST_ICL_DISCRETE_STATIC_INTERVAL_DEFAULT right_open_interval
#define BOOST_ICL_DISCRETE_STATIC_INTERVAL_BORDERS 2 //0=() 1=(] 2=[) 3=[]

#include <boost/icl/rational.hpp>

#include <boost/icl/detail/interval_morphism.hpp>
#include <boost/icl/interval_map.hpp>
#include "../test_laws.hpp"

using namespace std;
using namespace boost;
using namespace unit_test;
using namespace boost::icl;


BOOST_AUTO_TEST_CASE(test_intervals_4_changed_lib_defaults)
{
    typedef int T;
    typedef int U;
    typedef interval_map<T,U, total_absorber> IntervalMapT;
    typedef interval_set<T>                   IntervalSetT;
    typedef IntervalMapT::interval_type       IntervalT;

    typedef icl::map<int,int> MapII;

    //const bool xx = is_same< typename icl::map<int,int>::codomain_type, 
    //    typename codomain_type_of<icl::map<int,int> >::type >::value;


#if defined(BOOST_ICL_USE_STATIC_BOUNDED_INTERVALS) && !defined(BOOST_ICL_DISCRETE_STATIC_INTERVAL_DEFAULT)
    BOOST_CHECK( (boost::is_same<icl::interval<int   >::type, right_open_interval<int   > >::value) );
    BOOST_CHECK( (boost::is_same<icl::interval<double>::type, right_open_interval<double> >::value) );

    BOOST_CHECK_EQUAL( icl::interval<int>::open(0,2),       icl::construct<right_open_interval<int> >(1,2) );
    BOOST_CHECK_EQUAL( icl::interval<int>::left_open(0,1),  icl::construct<right_open_interval<int> >(1,2) );
    BOOST_CHECK_EQUAL( icl::interval<int>::right_open(1,2), icl::construct<right_open_interval<int> >(1,2) );
    BOOST_CHECK_EQUAL( icl::interval<int>::closed(1,1),     icl::construct<right_open_interval<int> >(1,2) );

    BOOST_CHECK_EQUAL( icl::interval<float>::right_open(1.0,2.0), icl::construct<right_open_interval<float> >(1.0,2.0) );
    //The next yields compiletime error: STATIC_ASSERTION_FAILURE
    //BOOST_CHECK_EQUAL( icl::interval<float>::left_open(1.0,2.0), icl::construct<right_open_interval<float> >(1.0,2.0) );
#endif

#if defined(BOOST_ICL_USE_STATIC_BOUNDED_INTERVALS) && defined(BOOST_ICL_DISCRETE_STATIC_INTERVAL_DEFAULT)
#   if defined(BOOST_ICL_DISCRETE_STATIC_INTERVAL_BORDERS) && (BOOST_ICL_DISCRETE_STATIC_INTERVAL_BORDERS == 0)
    cout << "discrete_interval == open_interval\n";
    BOOST_CHECK( (boost::is_same<icl::interval<int>::type, open_interval<int> >::value) );
    BOOST_CHECK_EQUAL( icl::interval<int>::open(0,2),       icl::construct<open_interval<int> >(0,2) );
    BOOST_CHECK_EQUAL( icl::interval<int>::left_open(0,1),  icl::construct<open_interval<int> >(0,2) );
    BOOST_CHECK_EQUAL( icl::interval<int>::right_open(1,2), icl::construct<open_interval<int> >(0,2) );
    BOOST_CHECK_EQUAL( icl::interval<int>::closed(1,1),     icl::construct<open_interval<int> >(0,2) );

#   elif defined(BOOST_ICL_DISCRETE_STATIC_INTERVAL_BORDERS) && (BOOST_ICL_DISCRETE_STATIC_INTERVAL_BORDERS == 1)
    cout << "discrete_interval == left_open_interval\n";
    BOOST_CHECK( (boost::is_same<icl::interval<int>::type, left_open_interval<int> >::value) );
    BOOST_CHECK_EQUAL( icl::interval<int>::open(0,2),       icl::construct<left_open_interval<int> >(0,1) );
    BOOST_CHECK_EQUAL( icl::interval<int>::left_open(0,1),  icl::construct<left_open_interval<int> >(0,1) );
    BOOST_CHECK_EQUAL( icl::interval<int>::right_open(1,2), icl::construct<left_open_interval<int> >(0,1) );
    BOOST_CHECK_EQUAL( icl::interval<int>::closed(1,1),     icl::construct<left_open_interval<int> >(0,1) );

#   elif defined(BOOST_ICL_DISCRETE_STATIC_INTERVAL_BORDERS) && (BOOST_ICL_DISCRETE_STATIC_INTERVAL_BORDERS == 2)
    cout << "discrete_interval == right_open_interval\n";
    BOOST_CHECK( (boost::is_same<icl::interval<int>::type, right_open_interval<int> >::value) );
    BOOST_CHECK_EQUAL( icl::interval<int>::open(0,2),       icl::construct<right_open_interval<int> >(1,2) );
    BOOST_CHECK_EQUAL( icl::interval<int>::left_open(0,1),  icl::construct<right_open_interval<int> >(1,2) );
    BOOST_CHECK_EQUAL( icl::interval<int>::right_open(1,2), icl::construct<right_open_interval<int> >(1,2) );
    BOOST_CHECK_EQUAL( icl::interval<int>::closed(1,1),     icl::construct<right_open_interval<int> >(1,2) );

#   elif defined(BOOST_ICL_DISCRETE_STATIC_INTERVAL_BORDERS) && (BOOST_ICL_DISCRETE_STATIC_INTERVAL_BORDERS == 3)
    cout << "discrete_interval == closed_interval\n";
    BOOST_CHECK( (boost::is_same<icl::interval<int>::type, closed_interval<int> >::value) );
    BOOST_CHECK_EQUAL( icl::interval<int>::open(0,2),       icl::construct<closed_interval<int> >(1,1) );
    BOOST_CHECK_EQUAL( icl::interval<int>::left_open(0,1),  icl::construct<closed_interval<int> >(1,1) );
    BOOST_CHECK_EQUAL( icl::interval<int>::right_open(1,2), icl::construct<closed_interval<int> >(1,1) );
    BOOST_CHECK_EQUAL( icl::interval<int>::closed(1,1),     icl::construct<closed_interval<int> >(1,1) );

#   else
    cout << "#else part, INTERVAL_BORDERS not in {0,1,2,3}\n";
#endif //defined(BOOST_ICL_USE_STATIC_BOUNDED_INTERVALS) && defined(BOOST_ICL_DISCRETE_STATIC_INTERVAL_DEFAULT)

#else
    BOOST_CHECK( (boost::is_same<icl::interval<int   >::type,   discrete_interval<int   > >::value) );
    BOOST_CHECK( (boost::is_same<icl::interval<double>::type, continuous_interval<double> >::value) );
#endif

}

