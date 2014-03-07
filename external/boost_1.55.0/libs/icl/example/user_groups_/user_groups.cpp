/*-----------------------------------------------------------------------------+    
Interval Container Library
Author: Joachim Faulhaber
Copyright (c) 2007-2009: Joachim Faulhaber
Copyright (c) 1999-2006: Cortex Software GmbH, Kantstrasse 57, Berlin
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
/** Example user_groups.cpp \file user_groups.cpp
    \brief Shows the availability of set operations on interval maps. 

    In the example there is a user group 'med_users' of a hosptial staff
    that has the authorisation to handle medical data of patients.
    User group 'admin_users' has access to administrative data like
    health insurance and invoice data.

    The membership for each user in one of the user groups has a time
    interval of validity. The group membership begins and ends.

    Using a union operation '+' we can have an overview over both
    user groups and the membership dates of employees.

    Computing an intersection '&' shows the super users. The persons
    that are members of both med_users and admin_users and the times
    of the joint memberships.
    
    \include user_groups_/user_groups.cpp
*/
//[example_user_groups
// The next line includes <boost/gregorian/date.hpp>
// and a few lines of adapter code.
#include <boost/icl/gregorian.hpp> 
#include <iostream>
#include <boost/icl/interval_map.hpp>

using namespace std;
using namespace boost::gregorian;
using namespace boost::icl;

// Type icl::set<string> collects the names a user group's members. Therefore
// it needs to implement operator += that performs a set union on overlap of 
// intervals.
typedef std::set<string> MemberSetT;

// boost::gregorian::date is the domain type the interval map. 
// It's key values are therefore time intervals: discrete_interval<date>. The content
// is the set of names: MemberSetT.
typedef interval_map<date, MemberSetT> MembershipT;

// Collect user groups for medical and administrative staff and perform
// union and intersection operations on the collected membership schedules.
void user_groups()
{
    MemberSetT mary_harry; 
    mary_harry.insert("Mary");
    mary_harry.insert("Harry");

    MemberSetT diana_susan; 
    diana_susan.insert("Diana");
    diana_susan.insert("Susan");

    MemberSetT chief_physician; 
    chief_physician.insert("Dr.Jekyll");

    MemberSetT director_of_admin; 
    director_of_admin.insert("Mr.Hyde");

    //----- Collecting members of user group: med_users -------------------
    MembershipT med_users;

    med_users.add( // add and element
      make_pair( 
        discrete_interval<date>::closed(
          from_string("2008-01-01"), from_string("2008-12-31")), mary_harry));

    med_users +=  // element addition can also be done via operator +=
      make_pair( 
        discrete_interval<date>::closed(
          from_string("2008-01-15"), from_string("2008-12-31")), 
          chief_physician);

    med_users +=
      make_pair( 
        discrete_interval<date>::closed(
          from_string("2008-02-01"), from_string("2008-10-15")), 
          director_of_admin);

    //----- Collecting members of user group: admin_users ------------------
    MembershipT admin_users;

    admin_users += // element addition can also be done via operator +=
      make_pair( 
        discrete_interval<date>::closed(
          from_string("2008-03-20"), from_string("2008-09-30")), diana_susan);

    admin_users +=
      make_pair( 
        discrete_interval<date>::closed(
          from_string("2008-01-15"), from_string("2008-12-31")), 
          chief_physician);

    admin_users +=
      make_pair( 
        discrete_interval<date>::closed(
          from_string("2008-02-01"), from_string("2008-10-15")), 
          director_of_admin);

    MembershipT all_users   = med_users + admin_users;

    MembershipT super_users = med_users & admin_users;

    MembershipT::iterator med_ = med_users.begin();
    cout << "----- Membership of medical staff -----------------------------------\n";
    while(med_ != med_users.end())
    {
        discrete_interval<date> when = (*med_).first;
        // Who is member of group med_users within the time interval 'when' ?
        MemberSetT who = (*med_++).second;
        cout << "[" << first(when) << " - " << last(when) << "]"
             << ": " << who << endl;
    }

    MembershipT::iterator admin_ = admin_users.begin();
    cout << "----- Membership of admin staff -------------------------------------\n";
    while(admin_ != admin_users.end())
    {
        discrete_interval<date> when = (*admin_).first;
        // Who is member of group admin_users within the time interval 'when' ?
        MemberSetT who = (*admin_++).second;
        cout << "[" << first(when) << " - " << last(when) << "]"
             << ": " << who << endl;
    }

    MembershipT::iterator all_ = all_users.begin();
    cout << "----- Membership of all users (med + admin) -------------------------\n";
    while(all_ != all_users.end())
    {
        discrete_interval<date> when = (*all_).first;
        // Who is member of group med_users OR admin_users ?
        MemberSetT who = (*all_++).second;
        cout << "[" << first(when) << " - " << last(when) << "]"
             << ": " << who << endl;
    }

    MembershipT::iterator super_ = super_users.begin();
    cout << "----- Membership of super users: intersection(med,admin) ------------\n";
    while(super_ != super_users.end())
    {
        discrete_interval<date> when = (*super_).first;
        // Who is member of group med_users AND admin_users ?
        MemberSetT who = (*super_++).second;
        cout << "[" << first(when) << " - " << last(when) << "]"
             << ": " << who << endl;
    }

}


int main()
{
    cout << ">>Interval Container Library: Sample user_groups.cpp <<\n";
    cout << "-------------------------------------------------------\n";
    user_groups();
    return 0;
}

// Program output:
/*-----------------------------------------------------------------------------
>>Interval Container Library: Sample user_groups.cpp <<
-------------------------------------------------------
----- Membership of medical staff -----------------------------------
[2008-Jan-01 - 2008-Jan-14]: Harry Mary
[2008-Jan-15 - 2008-Jan-31]: Dr.Jekyll Harry Mary
[2008-Feb-01 - 2008-Oct-15]: Dr.Jekyll Harry Mary Mr.Hyde
[2008-Oct-16 - 2008-Dec-31]: Dr.Jekyll Harry Mary
----- Membership of admin staff -------------------------------------
[2008-Jan-15 - 2008-Jan-31]: Dr.Jekyll
[2008-Feb-01 - 2008-Mar-19]: Dr.Jekyll Mr.Hyde
[2008-Mar-20 - 2008-Sep-30]: Diana Dr.Jekyll Mr.Hyde Susan
[2008-Oct-01 - 2008-Oct-15]: Dr.Jekyll Mr.Hyde
[2008-Oct-16 - 2008-Dec-31]: Dr.Jekyll
----- Membership of all users (med + admin) -------------------------
[2008-Jan-01 - 2008-Jan-14]: Harry Mary
[2008-Jan-15 - 2008-Jan-31]: Dr.Jekyll Harry Mary
[2008-Feb-01 - 2008-Mar-19]: Dr.Jekyll Harry Mary Mr.Hyde
[2008-Mar-20 - 2008-Sep-30]: Diana Dr.Jekyll Harry Mary Mr.Hyde Susan
[2008-Oct-01 - 2008-Oct-15]: Dr.Jekyll Harry Mary Mr.Hyde
[2008-Oct-16 - 2008-Dec-31]: Dr.Jekyll Harry Mary
----- Membership of super users: intersection(med,admin) ------------
[2008-Jan-15 - 2008-Jan-31]: Dr.Jekyll
[2008-Feb-01 - 2008-Oct-15]: Dr.Jekyll Mr.Hyde
[2008-Oct-16 - 2008-Dec-31]: Dr.Jekyll
-----------------------------------------------------------------------------*/
//]

