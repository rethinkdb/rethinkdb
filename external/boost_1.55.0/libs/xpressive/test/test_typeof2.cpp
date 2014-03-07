///////////////////////////////////////////////////////////////////////////////
// test_typeof2.cpp
//
//  Copyright 2008 David Jenkins. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_TYPEOF_LIMIT_SIZE 200
#define BOOST_TYPEOF_EMULATION 1

#include <string>
#include <map>
#include <list>
#include <stack>
#include <boost/version.hpp>
#include <boost/xpressive/xpressive_static.hpp>
#include <boost/xpressive/regex_actions.hpp>
#include <boost/xpressive/xpressive_typeof.hpp>
#include <boost/typeof/std/stack.hpp>
#include <boost/typeof/std/list.hpp>
#include <boost/test/unit_test.hpp>


// I couldn't find these registrations anywhere else, so I put them here
// They are necessary for this program to compile
#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::mpl::int_, (int))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::reference_wrapper, (typename))

// Here's the test for typeof registration, to be used on static regular expressions
#define TYPEOF_TEST(Expr) { BOOST_PROTO_AUTO(Dummy, Expr); }

namespace xp = boost::xpressive;

///////////////////////////////////////////////////////////////////////////////
// test_actions
//  regular expressions from test_actions.cpp
void test_actions()
{
    using namespace boost::xpressive;
    // regexes from test_actions.cpp
    std::string result;
    TYPEOF_TEST((+_w)[ xp::ref(result) += _ ] >> *(' ' >> (+_w)[ xp::ref(result) += ',' + _ ]));
    TYPEOF_TEST((+_w)[ xp::ref(result) += _ ] >> *(' ' >> (+_w)[ xp::ref(result) += ',' + _ ]) >> repeat<4>(_));
    std::list<int> result2;
    TYPEOF_TEST((+_d)[ xp::ref(result2)->*push_back( as<int>(_) ) ]
        >> *(' ' >> (+_d)[ xp::ref(result2)->*push_back( as<int>(_) ) ]));
    std::map<std::string, int> result3;
    TYPEOF_TEST(( (s1= +_w) >> "=>" >> (s2= +_d) )[ xp::ref(result3)[s1] = as<int>(s2) ]);
    placeholder< std::map<std::string, int> > const _map5 = {{}};
    TYPEOF_TEST(( (s1= +_w) >> "=>" >> (s2= +_d) )[ _map5[s1] = as<int>(s2) ]);

    smatch what;
    placeholder< std::map<std::string, int> > const _map6 = {{}};
    std::map<std::string, int> result6;
    what.let(_map6 = result6); // bind the argument!

    local<int> left, right;
    std::stack<int> stack_;
    reference<std::stack<int> > stack(stack_);
    cregex expression2, factor2, term2, group2;
    TYPEOF_TEST( '(' >> by_ref(expression2) >> ')');
    TYPEOF_TEST( (+_d)[ push(stack, as<int>(_)) ] | group2);
    TYPEOF_TEST(factor2 >> *(
            ('*' >> factor2)
                [ right = top(stack)
                , pop(stack)
                , left = top(stack)
                , pop(stack)
                , push(stack, left * right)
                ]
         ));
    TYPEOF_TEST(term2 >> *(
            ('+' >> term2)
                [ right = top(stack)
                , pop(stack)
                , left = top(stack)
                , pop(stack)
                , push(stack, left + right)
                ]
         ));
}


#ifndef BOOST_XPRESSIVE_NO_WREGEX
    struct City
    {
        std::wstring name;
        char const* nickname;
        int population;
    };
    BOOST_TYPEOF_REGISTER_TYPE(City)
#endif

///////////////////////////////////////////////////////////////////////////////
// test_symbols
//  regular expressions from test_symbols.cpp
void test_symbols()
{
    using namespace boost::xpressive;
    std::string result;
    std::map<std::string,std::string> map10;
    TYPEOF_TEST((a1=map10)[ xp::ref(result) = a1 ] >> *(' ' >> (a1=map10)[ xp::ref(result) += ',' + a1 ]));
    TYPEOF_TEST((a1=map10)[ xp::ref(result) = a1 ]
        >> *((a1=map10)[ xp::ref(result) += ',', xp::ref(result) += a1 ]));
    std::list<int> result12;
    std::map<std::string,int> map12;
    TYPEOF_TEST((a1=map12)[ xp::ref(result12)->*push_back( a1 ) ]
        >> *(' ' >> (a1=map12)[ xp::ref(result12)->*push_back( a1 ) ]));

    placeholder< std::map<std::string, int> > const _map13 = {};
    BOOST_PROTO_AUTO(pair13, ( (a1=map10) >> "=>" >> (a2= map12) )[ _map13[a1] = a2 ]);
    smatch what;
    std::map<std::string, int> result13;
    what.let(_map13 = result13);
    TYPEOF_TEST(pair13 >> *(+_s >> pair13));

    int result14 = 0;
    std::map<std::string,int> map1a;
    std::map<std::string,int> map2a;
    std::map<std::string,int> map3a;
    TYPEOF_TEST((a1=map1a)[ xp::ref(result14) += a1 ]
        >> (a2=map2a)[ xp::ref(result) += a2 ]
        >> (a3=map3a)[ xp::ref(result) += a3 ]
        );
        {
    TYPEOF_TEST(icase(a1= map10) [ xp::ref(result) = a1 ]
        >> repeat<3>( (' ' >> icase(a1= map10) [ xp::ref(result) += ',', xp::ref(result) += a1 ]) )
        );
    TYPEOF_TEST(*((a1= map1a) | (a1= map2a) | 'e') [ xp::ref(result) += (a1 | "9") ]);
        }
#ifndef BOOST_XPRESSIVE_NO_WREGEX
    City result17a, result17b;
    std::map<std::wstring, City> map17;
    TYPEOF_TEST((a1= map17)[ xp::ref(result17a) = a1 ] >> +_s
        >> (a1= map17)[ xp::ref(result17b) = a1 ]);
#else
    // This test is empty
#endif

}

bool three_or_six(xp::csub_match const &sub)
{
    return sub.length() == 3 || sub.length() == 6;
}

///////////////////////////////////////////////////////////////////////////////
// test_assert
//  regular expressions from test_assert.cpp
void test_assert()
{
    using namespace boost::xpressive;
    std::string result;
    TYPEOF_TEST((bow >> +_w >> eow)[ check(&three_or_six) ]);
    TYPEOF_TEST((bow >> +_w >> eow)[ check(length(_)==3 || length(_)==6) ]);
    int const days_per_month[] =
        {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 31, 31};
    mark_tag month(1), day(2);
    // Note: if you uncomment the lines below,
    // the BOOST_TYPEOF_LIMIT_SIZE is exceeded
    TYPEOF_TEST((
        // Month must be between 1 and 12 inclusive
        (month= _d >> !_d)     [ check(as<int>(_) >= 1
                                    && as<int>(_) <= 12) ]
        //>>  '/'
        //    // Day must be between 1 and 31 inclusive
        //>>  (day=   _d >> !_d)     [ check(as<int>(_) >= 1
        //                                && as<int>(_) <= 31) ]
        //>>  '/'
        //    // Only consider years between 1970 and 2038
        //>>  (_d >> _d >> _d >> _d) [ check(as<int>(_) >= 1970
        //                                && as<int>(_) <= 2038) ]
        )
        // Ensure the month actually has that many days.
        [ check( ref(days_per_month)[as<int>(month)-1] >= as<int>(day) ) ]);
}

