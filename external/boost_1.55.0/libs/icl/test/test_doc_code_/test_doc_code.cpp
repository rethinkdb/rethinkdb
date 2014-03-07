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

#include <boost/icl/rational.hpp>

#include <boost/icl/detail/interval_morphism.hpp>
#include <boost/icl/interval_map.hpp>
#include "../test_laws.hpp"

using namespace std;
using namespace boost;
using namespace unit_test;
using namespace boost::icl;

BOOST_AUTO_TEST_CASE(intro_sample_telecast)
{
    // Switch on my favorite telecasts using an interval_set
    interval<int>::type news(2000, 2015);
    interval<int>::type talk_show(2245, 2330);
    interval_set<int> myTvProgram;
    myTvProgram.add(news).add(talk_show);

    // Iterating over elements (seconds) would be silly ...
    for(interval_set<int>::iterator telecast = myTvProgram.begin(); 
        telecast != myTvProgram.end(); ++telecast)
        //...so this iterates over intervals
        //TV.switch_on(*telecast);
        cout << *telecast;

    cout << endl;
}

BOOST_AUTO_TEST_CASE(interface_sample_identifiers)
{
    typedef interval_set<std::string, less, continuous_interval<std::string> > IdentifiersT;
    IdentifiersT identifiers, excluded;

    // special identifiers shall be excluded
    identifiers += continuous_interval<std::string>::right_open("a", "c");
    identifiers -= std::string("boost");
    cout << "identifiers: " << identifiers << endl;

    excluded = IdentifiersT(icl::hull(identifiers)) - identifiers;
    cout << "excluded   : " << excluded << endl;

    if(icl::contains(identifiers, std::string("boost")))
        cout << "error, identifiers.contains('boost')\n";
}

BOOST_AUTO_TEST_CASE(function_reference_element_iteration)
{
    // begin of doc code -------------------------------------------------------
    interval_set<int> inter_set;
    inter_set.add(interval<int>::right_open(0,3))
             .add(interval<int>::right_open(7,9));

    for(interval_set<int>::element_const_iterator creeper = elements_begin(inter_set); 
        creeper != elements_end(inter_set); ++creeper)
        cout << *creeper << " ";
    cout << endl;
    //Program output: 0 1 2 7 8

    for(interval_set<int>::element_reverse_iterator repeerc = elements_rbegin(inter_set); 
        repeerc != elements_rend(inter_set); ++repeerc)
        cout << *repeerc << " ";
    cout << endl;
    //Program output: 8 7 2 1 0
    // end of doc code ---------------------------------------------------------

    // Testcode
    std::stringstream result;
    for(interval_set<int>::element_iterator creeper2 = elements_begin(inter_set); 
        creeper2 != elements_end(inter_set); ++creeper2)
        result << *creeper2 << " ";

    BOOST_CHECK_EQUAL(result.str(), std::string("0 1 2 7 8 "));

    std::stringstream tluser;
    for(interval_set<int>::element_const_reverse_iterator repeerc2 
            = elements_rbegin(const_cast<const interval_set<int>&>(inter_set)); 
        repeerc2 != elements_rend(const_cast<const interval_set<int>&>(inter_set)); ++repeerc2)
        tluser << *repeerc2 << " ";

    BOOST_CHECK_EQUAL(tluser.str(), std::string("8 7 2 1 0 "));
}

