//  switch_test.cpp  -- The Boost Lambda Library --------------------------
//
// Copyright (C) 2000-2003 Jaakko Jarvi (jaakko.jarvi@cs.utu.fi)
// Copyright (C) 2000-2003 Gary Powell (powellg@amazon.com)
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org

// -----------------------------------------------------------------------


#include <boost/test/minimal.hpp>    // see "Header Implementation Option"


#include "boost/lambda/lambda.hpp"
#include "boost/lambda/if.hpp"
#include "boost/lambda/switch.hpp"

#include <iostream>
#include <algorithm>
#include <vector>
#include <string>



// Check that elements 0 -- index are 1, and the rest are 0
bool check(const std::vector<int>& v, int index) {
  using namespace boost::lambda;
  int counter = 0;
  std::vector<int>::const_iterator 
    result = std::find_if(v.begin(), v.end(),
                     ! if_then_else_return(
                         var(counter)++ <= index,
                         _1 == 1,
                         _1 == 0)
                    );
  return result == v.end();
}

  

void do_switch_no_defaults_tests() {

  using namespace boost::lambda;  

  int i = 0;
  std::vector<int> v,w;

  // elements from 0 to 9
  std::generate_n(std::back_inserter(v),
                  10, 
                  var(i)++);
  std::fill_n(std::back_inserter(w), 10, 0);

  // ---
  std::for_each(v.begin(), v.end(),
    switch_statement( 
      _1,
      case_statement<0>(++var(w[0]))
    )
  );
  
  BOOST_CHECK(check(w, 0));
  std::fill_n(w.begin(), 10, 0);

  // ---
  std::for_each(v.begin(), v.end(),
    switch_statement( 
      _1,
      case_statement<0>(++var(w[0])),
      case_statement<1>(++var(w[1]))
    )
  );
  
  BOOST_CHECK(check(w, 1));
  std::fill_n(w.begin(), 10, 0);

  // ---
  std::for_each(v.begin(), v.end(),
    switch_statement( 
      _1,
      case_statement<0>(++var(w[0])),
      case_statement<1>(++var(w[1])),
      case_statement<2>(++var(w[2]))
     )
  );
  
  BOOST_CHECK(check(w, 2));
  std::fill_n(w.begin(), 10, 0);

  // ---
  std::for_each(v.begin(), v.end(),
    switch_statement( 
      _1,
      case_statement<0>(++var(w[0])),
      case_statement<1>(++var(w[1])),
      case_statement<2>(++var(w[2])),
      case_statement<3>(++var(w[3]))
    )
  );
  
  BOOST_CHECK(check(w, 3));
  std::fill_n(w.begin(), 10, 0);

  // ---
  std::for_each(v.begin(), v.end(),
    switch_statement( 
      _1,
      case_statement<0>(++var(w[0])),
      case_statement<1>(++var(w[1])),
      case_statement<2>(++var(w[2])),
      case_statement<3>(++var(w[3])),
      case_statement<4>(++var(w[4]))
    )
  );
  
  BOOST_CHECK(check(w, 4));
  std::fill_n(w.begin(), 10, 0);

  // ---
  std::for_each(v.begin(), v.end(),
    switch_statement( 
      _1,
      case_statement<0>(++var(w[0])),
      case_statement<1>(++var(w[1])),
      case_statement<2>(++var(w[2])),
      case_statement<3>(++var(w[3])),
      case_statement<4>(++var(w[4])),
      case_statement<5>(++var(w[5]))
    )
  );
  
  BOOST_CHECK(check(w, 5));
  std::fill_n(w.begin(), 10, 0);

  // ---
  std::for_each(v.begin(), v.end(),
    switch_statement( 
      _1,
      case_statement<0>(++var(w[0])),
      case_statement<1>(++var(w[1])),
      case_statement<2>(++var(w[2])),
      case_statement<3>(++var(w[3])),
      case_statement<4>(++var(w[4])),
      case_statement<5>(++var(w[5])),
      case_statement<6>(++var(w[6]))
    )
  );
  
  BOOST_CHECK(check(w, 6));
  std::fill_n(w.begin(), 10, 0);

  // ---
  std::for_each(v.begin(), v.end(),
    switch_statement( 
      _1,
      case_statement<0>(++var(w[0])),
      case_statement<1>(++var(w[1])),
      case_statement<2>(++var(w[2])),
      case_statement<3>(++var(w[3])),
      case_statement<4>(++var(w[4])),
      case_statement<5>(++var(w[5])),
      case_statement<6>(++var(w[6])),
      case_statement<7>(++var(w[7]))
    )
  );
  
  BOOST_CHECK(check(w, 7));
  std::fill_n(w.begin(), 10, 0);

  // ---
  std::for_each(v.begin(), v.end(),
    switch_statement( 
      _1,
      case_statement<0>(++var(w[0])),
      case_statement<1>(++var(w[1])),
      case_statement<2>(++var(w[2])),
      case_statement<3>(++var(w[3])),
      case_statement<4>(++var(w[4])),
      case_statement<5>(++var(w[5])),
      case_statement<6>(++var(w[6])),
      case_statement<7>(++var(w[7])),
      case_statement<8>(++var(w[8]))
    )
  );
  
  BOOST_CHECK(check(w, 8));
  std::fill_n(w.begin(), 10, 0);

}


void do_switch_yes_defaults_tests() {

  using namespace boost::lambda;  

  int i = 0;
  std::vector<int> v,w;

  // elements from 0 to 9
  std::generate_n(std::back_inserter(v),
                  10, 
                  var(i)++);
  std::fill_n(std::back_inserter(w), 10, 0);

  int default_count;
  // ---
  default_count = 0;
  std::for_each(v.begin(), v.end(),
    switch_statement( 
      _1,
      default_statement(++var(default_count))
    )
  );
  
  BOOST_CHECK(check(w, -1));
  BOOST_CHECK(default_count == 10);
  std::fill_n(w.begin(), 10, 0);

  // ---
  default_count = 0;
  std::for_each(v.begin(), v.end(),
    switch_statement( 
      _1,
      case_statement<0>(++var(w[0])),
      default_statement(++var(default_count))
    )
  );
  
  BOOST_CHECK(check(w, 0));
  BOOST_CHECK(default_count == 9);
  std::fill_n(w.begin(), 10, 0);

  // ---
  default_count = 0;
  std::for_each(v.begin(), v.end(),
    switch_statement( 
      _1,
      case_statement<0>(++var(w[0])),
      case_statement<1>(++var(w[1])),
      default_statement(++var(default_count))
     )
  );
  
  BOOST_CHECK(check(w, 1));
  BOOST_CHECK(default_count == 8);
  std::fill_n(w.begin(), 10, 0);

  // ---
  default_count = 0;
  std::for_each(v.begin(), v.end(),
    switch_statement( 
      _1,
      case_statement<0>(++var(w[0])),
      case_statement<1>(++var(w[1])),
      case_statement<2>(++var(w[2])),
      default_statement(++var(default_count))
    )
  );
  
  BOOST_CHECK(check(w, 2));
  BOOST_CHECK(default_count == 7);
  std::fill_n(w.begin(), 10, 0);

  // ---
  default_count = 0;
  std::for_each(v.begin(), v.end(),
    switch_statement( 
      _1,
      case_statement<0>(++var(w[0])),
      case_statement<1>(++var(w[1])),
      case_statement<2>(++var(w[2])),
      case_statement<3>(++var(w[3])),
      default_statement(++var(default_count))
    )
  );
  
  BOOST_CHECK(check(w, 3));
  BOOST_CHECK(default_count == 6);
  std::fill_n(w.begin(), 10, 0);

  // ---
  default_count = 0;
  std::for_each(v.begin(), v.end(),
    switch_statement( 
      _1,
      case_statement<0>(++var(w[0])),
      case_statement<1>(++var(w[1])),
      case_statement<2>(++var(w[2])),
      case_statement<3>(++var(w[3])),
      case_statement<4>(++var(w[4])),
      default_statement(++var(default_count))
    )
  );
  
  BOOST_CHECK(check(w, 4));
  BOOST_CHECK(default_count == 5);
  std::fill_n(w.begin(), 10, 0);

  // ---
  default_count = 0;
  std::for_each(v.begin(), v.end(),
    switch_statement( 
      _1,
      case_statement<0>(++var(w[0])),
      case_statement<1>(++var(w[1])),
      case_statement<2>(++var(w[2])),
      case_statement<3>(++var(w[3])),
      case_statement<4>(++var(w[4])),
      case_statement<5>(++var(w[5])),
      default_statement(++var(default_count))
    )
  );
  
  BOOST_CHECK(check(w, 5));
  BOOST_CHECK(default_count == 4);
  std::fill_n(w.begin(), 10, 0);

  // ---
  default_count = 0;
  std::for_each(v.begin(), v.end(),
    switch_statement( 
      _1,
      case_statement<0>(++var(w[0])),
      case_statement<1>(++var(w[1])),
      case_statement<2>(++var(w[2])),
      case_statement<3>(++var(w[3])),
      case_statement<4>(++var(w[4])),
      case_statement<5>(++var(w[5])),
      case_statement<6>(++var(w[6])),
      default_statement(++var(default_count))
    )
  );
  
  BOOST_CHECK(check(w, 6));
  BOOST_CHECK(default_count == 3);
  std::fill_n(w.begin(), 10, 0);

  // ---
  default_count = 0;
  std::for_each(v.begin(), v.end(),
    switch_statement( 
      _1,
      case_statement<0>(++var(w[0])),
      case_statement<1>(++var(w[1])),
      case_statement<2>(++var(w[2])),
      case_statement<3>(++var(w[3])),
      case_statement<4>(++var(w[4])),
      case_statement<5>(++var(w[5])),
      case_statement<6>(++var(w[6])),
      case_statement<7>(++var(w[7])),
      default_statement(++var(default_count))
    )
  );
  
  BOOST_CHECK(check(w, 7));
  BOOST_CHECK(default_count == 2);
  std::fill_n(w.begin(), 10, 0);

}

void test_empty_cases() {

  using namespace boost::lambda;  

  // ---
  switch_statement( 
      _1,
      default_statement()
  )(make_const(1));

  switch_statement( 
      _1,
      case_statement<1>()
  )(make_const(1));

}

int test_main(int, char* []) {

  do_switch_no_defaults_tests();
  do_switch_yes_defaults_tests();

  test_empty_cases();

  return EXIT_SUCCESS;

}

