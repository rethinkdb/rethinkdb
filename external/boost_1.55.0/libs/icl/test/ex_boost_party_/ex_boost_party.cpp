/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#define BOOST_TEST_MODULE icl::example_boost_party unit test

#include <libs/icl/test/disable_test_warnings.hpp>
#include "../unit_test_unwarned.hpp"
//#include <boost/icl/set.hpp> // Needed for implicit calls of operator << on
//JODO CLANG                   // GuestSets via test macros.

//------------------------------------------------------------------------------
// begin example code. return value added to function boost_party
//------------------------------------------------------------------------------
#include <boost/icl/ptime.hpp> 
#include <iostream>
#include <boost/icl/interval_map.hpp>

using namespace std;
using namespace boost::posix_time;
using namespace boost::icl;

// Type set<string> collects the names of party guests. Since std::set is
// a model of the itl's set concept, the concept provides an operator += 
// that performs a set union on overlap of intervals.
typedef std::set<string> GuestSetT;

interval_map<ptime, GuestSetT> boost_party()
{
    GuestSetT mary_harry; 
    mary_harry.insert("Mary");
    mary_harry.insert("Harry");

    GuestSetT diana_susan; 
    diana_susan.insert("Diana");
    diana_susan.insert("Susan");

    GuestSetT peter; 
    peter.insert("Peter");

    // A party is an interval map that maps time intervals to sets of guests
    interval_map<ptime, GuestSetT> party;

    party.add( // add and element
      make_pair( 
        interval<ptime>::right_open(
          time_from_string("2008-05-20 19:30"), 
          time_from_string("2008-05-20 23:00")), 
        mary_harry));

    party += // element addition can also be done via operator +=
      make_pair( 
        interval<ptime>::right_open(
          time_from_string("2008-05-20 20:10"), 
          time_from_string("2008-05-21 00:00")), 
        diana_susan);

    party +=
      make_pair( 
        interval<ptime>::right_open(
          time_from_string("2008-05-20 22:15"), 
          time_from_string("2008-05-21 00:30")), 
        peter);


    interval_map<ptime, GuestSetT>::iterator it = party.begin();
    cout << "----- History of party guests -------------------------\n";
    while(it != party.end())
    {
        interval<ptime>::type when = it->first;
        // Who is at the party within the time interval 'when' ?
        GuestSetT who = (*it++).second;
        cout << when << ": " << who << endl;
    }

    return party;
}
//------------------------------------------------------------------------------
// end example code
//------------------------------------------------------------------------------

typedef interval_map<ptime, GuestSetT> PartyHistory;

typedef PartyHistory::segment_type SegmentT;

SegmentT episode(const char* from, const char* to, GuestSetT guests)
{
    return make_pair( interval<ptime>
                      ::right_open( time_from_string(from)
                                  , time_from_string(to)   )
                    , guests);
}

PartyHistory check_party()
{
    GuestSetT mary_harry; 
    mary_harry.insert("Mary");
    mary_harry.insert("Harry");

    GuestSetT diana_susan; 
    diana_susan.insert("Diana");
    diana_susan.insert("Susan");

    GuestSetT peter; 
    peter.insert("Peter");

    GuestSetT Diana_Harry_Mary_Susan       = mary_harry + diana_susan;
    GuestSetT Diana_Harry_Mary_Peter_Susan = Diana_Harry_Mary_Susan + peter;
    GuestSetT Diana_Peter_Susan            = Diana_Harry_Mary_Peter_Susan - mary_harry;

    PartyHistory party;

    party += episode("2008-05-20 19:30", "2008-05-20 20:10", mary_harry);
    party += episode("2008-05-20 20:10", "2008-05-20 22:15", Diana_Harry_Mary_Susan);
    party += episode("2008-05-20 22:15", "2008-05-20 23:00", Diana_Harry_Mary_Peter_Susan);
    party += episode("2008-05-20 23:00", "2008-05-21 00:00", Diana_Peter_Susan);
    party += episode("2008-05-21 00:00", "2008-05-21 00:30", peter);

    return party;
}

BOOST_AUTO_TEST_CASE(icl_example_boost_party)
{
    PartyHistory party1 = boost_party();
    PartyHistory party2 = check_party();
    bool party_equality = (party1==party2);
    BOOST_CHECK(party_equality);
    //BOOST_CHECK_EQUAL(boost_party(), check_party());
}


