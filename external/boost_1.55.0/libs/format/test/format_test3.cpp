// ------------------------------------------------------------------------------
// format_test3.cpp :  complicated format strings  and / or advanced uses
// ------------------------------------------------------------------------------

//  Copyright Samuel Krempp 2003. Use, modification, and distribution are
//  subject to the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// see http://www.boost.org/libs/format for library home page

// ------------------------------------------------------------------------------

#define BOOST_FORMAT_STATIC_STREAM
#include "boost/format.hpp"

#include <iostream> 
#include <iomanip>

#define BOOST_INCLUDE_MAIN 
#include <boost/test/test_tools.hpp>

struct Rational {
  int n,d;
  Rational (int an, int ad) : n(an), d(ad) {}
};

std::ostream& operator<<( std::ostream& os, const Rational& r) {
  os << r.n << "/" << r.d;
  return os;
}

int test_main(int, char* [])
{
    using namespace std;
    using boost::format;
    using boost::io::group;
    using boost::str;

    string s, s2;
    // special paddings 
    s = str( format("[%=6s] [%+6s] [%+6s] [% 6s] [%+6s]\n") 
                      % 123
                      % group(internal, setfill('W'), 234)
                      % group(internal, setfill('X'), -345)
                      % group(setfill('Y'), 456)
                      % group(setfill('Z'), -10 ) );

    if(s != "[  123 ] [+WW234] [-XX345] [YY 456] [ZZZ-10]\n" ) {
      cerr << s ;
      BOOST_ERROR("formatting error. (with special paddings)");
    }

    s = str( format("[% 6.8s] [% 8.6s] [% 7.7s]\n") 
                      % group(internal, setfill('x'), Rational(12345,54321))
                      % group(internal, setfill('x'), Rational(123,45))
                      % group(internal, setfill('x'), Rational(123,321))
             );
    if(s != (s2="[ 12345/5] [ xx123/4] [ 123/32]\n" )) {
        cerr << s << s2;
      BOOST_ERROR("formatting error. (with special paddings)");
    }

    s = str( format("[% 6.8s] [% 6.8s] [% 6.8s] [% 6.8s] [%6.8s]\n") 
                      % 1234567897
                      % group(setfill('x'), 12)
                      % group(internal, setfill('x'), 12)
                      % group(internal, setfill('x'), 1234567890)
                      % group(internal, setfill('x'), 123456) 
             );
    if(s != (s2="[ 1234567] [xxx 12] [ xxx12] [ 1234567] [123456]\n") ) {
        cerr << s << s2;
      BOOST_ERROR("formatting error. (with special paddings)");
    }

    s = str( format("[% 8.6s] [% 6.4s] [% 6.4s] [% 8.6s] [% 8.6s]\n")
                      % 1234567897
                      % group(setfill('x'), 12)
                      % group(internal, setfill('x'), 12)
                      % group(internal, setfill('x'), 1234567890)
                      % group(internal, setfill('x'), 12345) 
             );
    if(s != (s2="[   12345] [xxx 12] [ xxx12] [ xx12345] [ xx12345]\n") ) {
        cerr << s << s2;
      BOOST_ERROR("formatting error. (with special paddings)");
    }

    // nesting formats :
    s = str( format("%2$014x [%1%] %|2$05|\n") % (format("%05s / %s") % -18 % 7)
             %group(showbase, -100)
             );
    if( s != "0x0000ffffff9c [-0018 / 7] -0100\n" ){
      cerr << s ;
      BOOST_ERROR("nesting did not work");
    }

    // bind args, and various arguments counts :
    {
        boost::format bf("%1% %4% %1%");
        bf.bind_arg(1, "one") % 2 % "three" ;
        BOOST_CHECK_EQUAL(bf.expected_args(), 4);
        BOOST_CHECK_EQUAL(bf.fed_args(), 2);
        BOOST_CHECK_EQUAL(bf.bound_args(), 1);
        BOOST_CHECK_EQUAL(bf.remaining_args(), 1);
        BOOST_CHECK_EQUAL(bf.cur_arg(), 4);
        bf.clear_binds();
        bf % "one" % 2 % "three" ;
        BOOST_CHECK_EQUAL(bf.expected_args(), 4);
        BOOST_CHECK_EQUAL(bf.fed_args(), 3);
        BOOST_CHECK_EQUAL(bf.bound_args(), 0);
        BOOST_CHECK_EQUAL(bf.remaining_args(), 1);
        BOOST_CHECK_EQUAL(bf.cur_arg(), 4);
    }
    // testcase for bug reported at 
    // http://lists.boost.org/boost-users/2006/05/19723.php
    {
        format f("%40t%1%");
        int x = 0;
        f.bind_arg(1, x);
        f.clear();
    }

    // testcase for bug reported at
    // http://lists.boost.org/boost-users/2005/11/15454.php
    std::string l_param;
    std::string l_str = (boost::format("here is an empty string: %1%") % l_param).str(); 

    // testcase for SourceForge bug #1506914
    std::string arg; // empty string  
    s = str(format("%=8s") % arg);

    return 0;
}
