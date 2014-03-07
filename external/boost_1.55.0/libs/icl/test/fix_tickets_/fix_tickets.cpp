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

//#define BOOST_ICL_USE_STATIC_BOUNDED_INTERVALS
#include <boost/icl/interval_map.hpp>
#include <boost/icl/split_interval_map.hpp>
#include <boost/icl/separate_interval_set.hpp>
#include <boost/icl/split_interval_set.hpp>

using namespace std;
using namespace boost;
using namespace unit_test;
using namespace boost::icl;


BOOST_AUTO_TEST_CASE(ticket_5482)
{
    typedef interval_map<int,int,partial_absorber,std::less>    m1_t;
    typedef interval_map<int,int,partial_absorber,std::greater> m2_t;
    m1_t m1;
    m2_t m2;
    m1.insert(make_pair(m1_t::interval_type(1), 20));
    m1.insert(make_pair(m1_t::interval_type(2), 20));
    m1.insert(make_pair(m1_t::interval_type(3), 20));

    m2.insert(make_pair(m2_t::interval_type(1), 20));
    m2.insert(make_pair(m2_t::interval_type(2), 20));
    m2.insert(make_pair(m2_t::interval_type(3), 20));

    BOOST_CHECK_EQUAL(m1.iterative_size(), m2.iterative_size());
    BOOST_CHECK_EQUAL(m1.iterative_size(), 1);
    BOOST_CHECK_EQUAL(m2.iterative_size(), 1);
}


#include <boost/cstdint.hpp>
BOOST_AUTO_TEST_CASE(ticket_5559_Denis)
{
    //Submitted by Denis
    typedef boost::icl::interval_set<boost::uint32_t, std::greater> Set;
    const uint32_t ui32_max = (std::numeric_limits<uint32_t>::max)();

    Set q1( Set::interval_type::closed(ui32_max, 0) );
    Set q5( Set::interval_type::closed(0, 0) );

    BOOST_CHECK_EQUAL(q1, q1+q5);
}

//------------------------------------------------------------------------------
// Ticket #6095 Marvin Sielenkemper
//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(ticket_6095_Marvin_Sielenkemper_isEmptyTest)
{
    typedef int                         Value;
    typedef boost::icl::interval<Value> Interval;
    typedef std::numeric_limits<Value>  Limits;

    Value const max((Limits::max)());

    Interval::interval_type piff = Interval::open(max,     max);

    BOOST_CHECK(!icl::is_empty(Interval::open(max - 2, max)));
    BOOST_CHECK( icl::is_empty(Interval::open(max - 1, max)));
    BOOST_CHECK( icl::is_empty(Interval::open(max,     max)));
}

BOOST_AUTO_TEST_CASE(ticket_6095_Marvin_Sielenkemper_totalRangeTest)
{
    typedef int                                                              Value;
    typedef boost::icl::interval<Value>                                      Interval;
    typedef std::numeric_limits<Value>                                       Limits;
    typedef boost::icl::interval_map<Value, int, boost::icl::total_enricher> Container;

    Value const min((Limits::min)());
    Value const max((Limits::max)());

    boost::icl::interval_map<Value, int, boost::icl::total_enricher> intervals;

    intervals += std::make_pair(Interval::closed(min, max), 0);
    intervals += std::make_pair(Interval::right_open(0, 10),  3);

    BOOST_CHECK_EQUAL(intervals.iterative_size(), 3);
}
