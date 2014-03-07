// -- exception_test.cpp -- The Boost Lambda Library ------------------
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

#include "boost/lambda/exceptions.hpp"

#include "boost/lambda/bind.hpp"

#include<iostream>
#include<algorithm>
#include <cstdlib>

#include <iostream>

using namespace boost::lambda;
using namespace std;

// to prevent unused variables warnings
template <class T> void dummy(const T&) {}

void erroneous_exception_related_lambda_expressions() {

  int i = 0;
  dummy(i);

  // Uncommenting any of the below code lines should result in a compile
  // time error

  // this should fail (a rethrow binder outside of catch
  //  rethrow()();
 
  // this should fail too for the same reason
  //  try_catch(rethrow(), catch_all(cout << constant("Howdy")))();

  // this fails too (_e outside of catch_exception)
  // (_1 + _2 + _e)(i, i, i); 

  // and this (_e outside of catch_exception)
  //  try_catch( throw_exception(1), catch_all(cout << _e));

  // and this (_3 in catch_exception
  //   try_catch( throw_exception(1), catch_exception<int>(cout << _3));
}


class A1 {};
class A2 {};
class A3 {};
class A4 {};
class A5 {};
class A6 {};
class A7 {};
class A8 {};
class A9 {};

void throw_AX(int j) {
  int i = j;
  switch(i) {
    case 1: throw A1();
    case 2: throw A2();
    case 3: throw A3();
    case 4: throw A4();
    case 5: throw A5();
    case 6: throw A6();
    case 7: throw A7();
    case 8: throw A8();
    case 9: throw A9();
  }  
}

void test_different_number_of_catch_blocks() {

  int ecount;

// no catch(...) cases


  ecount = 0;
  for(int i=1; i<=1; i++) 
  {
    try_catch( 
      bind(throw_AX, _1),
      catch_exception<A1>( 
        var(ecount)++
      )
    )(i);
  }
  BOOST_CHECK(ecount == 1);

  ecount = 0;
  for(int i=1; i<=2; i++) 
  {
    try_catch( 
      bind(throw_AX, _1),
      catch_exception<A1>( 
        var(ecount)++
      ),
      catch_exception<A2>( 
        var(ecount)++
      )
    )(i);
  }
  BOOST_CHECK(ecount == 2);

  ecount = 0;
  for(int i=1; i<=3; i++) 
  {
    try_catch( 
      bind(throw_AX, _1),
      catch_exception<A1>( 
        var(ecount)++
      ),
      catch_exception<A2>( 
        var(ecount)++
      ),
      catch_exception<A3>( 
        var(ecount)++
      )
    )(i);
  }
  BOOST_CHECK(ecount == 3);

  ecount = 0;
  for(int i=1; i<=4; i++) 
  {
    try_catch( 
      bind(throw_AX, _1),
      catch_exception<A1>( 
        var(ecount)++
      ),
      catch_exception<A2>( 
        var(ecount)++
      ),
      catch_exception<A3>( 
        var(ecount)++
      ),
      catch_exception<A4>( 
        var(ecount)++
      )
    )(i);
  }
  BOOST_CHECK(ecount == 4);

  ecount = 0;
  for(int i=1; i<=5; i++) 
  {
    try_catch( 
      bind(throw_AX, _1),
      catch_exception<A1>( 
        var(ecount)++
      ),
      catch_exception<A2>( 
        var(ecount)++
      ),
      catch_exception<A3>( 
        var(ecount)++
      ),
      catch_exception<A4>( 
        var(ecount)++
      ),
      catch_exception<A5>( 
        var(ecount)++
      )
    )(i);
  }
  BOOST_CHECK(ecount == 5);

  ecount = 0;
  for(int i=1; i<=6; i++) 
  {
    try_catch( 
      bind(throw_AX, _1),
      catch_exception<A1>( 
        var(ecount)++
      ),
      catch_exception<A2>( 
        var(ecount)++
      ),
      catch_exception<A3>( 
        var(ecount)++
      ),
      catch_exception<A4>( 
        var(ecount)++
      ),
      catch_exception<A5>( 
        var(ecount)++
      ),
      catch_exception<A6>( 
        var(ecount)++
      )
    )(i);
  }
  BOOST_CHECK(ecount == 6);

  ecount = 0;
  for(int i=1; i<=7; i++) 
  {
    try_catch( 
      bind(throw_AX, _1),
      catch_exception<A1>( 
        var(ecount)++
      ),
      catch_exception<A2>( 
        var(ecount)++
      ),
      catch_exception<A3>( 
        var(ecount)++
      ),
      catch_exception<A4>( 
        var(ecount)++
      ),
      catch_exception<A5>( 
        var(ecount)++
      ),
      catch_exception<A6>( 
        var(ecount)++
      ),
      catch_exception<A7>( 
        var(ecount)++
      )
    )(i);
  }
  BOOST_CHECK(ecount == 7);

  ecount = 0;
  for(int i=1; i<=8; i++) 
  {
    try_catch( 
      bind(throw_AX, _1),
      catch_exception<A1>( 
        var(ecount)++
      ),
      catch_exception<A2>( 
        var(ecount)++
      ),
      catch_exception<A3>( 
        var(ecount)++
      ),
      catch_exception<A4>( 
        var(ecount)++
      ),
      catch_exception<A5>( 
        var(ecount)++
      ),
      catch_exception<A6>( 
        var(ecount)++
      ),
      catch_exception<A7>( 
        var(ecount)++
      ),
      catch_exception<A8>( 
        var(ecount)++
      )
    )(i);
  }
  BOOST_CHECK(ecount == 8);

  ecount = 0;
  for(int i=1; i<=9; i++) 
  {
    try_catch( 
      bind(throw_AX, _1),
      catch_exception<A1>( 
        var(ecount)++
      ),
      catch_exception<A2>( 
        var(ecount)++
      ),
      catch_exception<A3>( 
        var(ecount)++
      ),
      catch_exception<A4>( 
        var(ecount)++
      ),
      catch_exception<A5>( 
        var(ecount)++
      ),
      catch_exception<A6>( 
        var(ecount)++
      ),
      catch_exception<A7>( 
        var(ecount)++
      ),
      catch_exception<A8>( 
        var(ecount)++
      ),
      catch_exception<A9>( 
        var(ecount)++
      )
    )(i);
  }
  BOOST_CHECK(ecount == 9);


  // with catch(...) blocks

  ecount = 0;
  for(int i=1; i<=1; i++) 
  {
    try_catch( 
      bind(throw_AX, _1),
      catch_all( 
        var(ecount)++
      )
    )(i);
  }
  BOOST_CHECK(ecount == 1);

  ecount = 0;
  for(int i=1; i<=2; i++) 
  {
    try_catch( 
      bind(throw_AX, _1),
      catch_exception<A1>( 
        var(ecount)++
      ),
      catch_all( 
        var(ecount)++
      )
    )(i);
  }
  BOOST_CHECK(ecount == 2);

  ecount = 0;
  for(int i=1; i<=3; i++) 
  {
    try_catch( 
      bind(throw_AX, _1),
      catch_exception<A1>( 
        var(ecount)++
      ),
      catch_exception<A2>( 
        var(ecount)++
      ),
      catch_all( 
        var(ecount)++
      )
    )(i);
  }
  BOOST_CHECK(ecount == 3);

  ecount = 0;
  for(int i=1; i<=4; i++) 
  {
    try_catch( 
      bind(throw_AX, _1),
      catch_exception<A1>( 
        var(ecount)++
      ),
      catch_exception<A2>( 
        var(ecount)++
      ),
      catch_exception<A3>( 
        var(ecount)++
      ),
      catch_all( 
        var(ecount)++
      )
    )(i);
  }
  BOOST_CHECK(ecount == 4);

  ecount = 0;
  for(int i=1; i<=5; i++) 
  {
    try_catch( 
      bind(throw_AX, _1),
      catch_exception<A1>( 
        var(ecount)++
      ),
      catch_exception<A2>( 
        var(ecount)++
      ),
      catch_exception<A3>( 
        var(ecount)++
      ),
      catch_exception<A4>( 
        var(ecount)++
      ),
      catch_all( 
        var(ecount)++
      )
    )(i);
  }
  BOOST_CHECK(ecount == 5);

  ecount = 0;
  for(int i=1; i<=6; i++) 
  {
    try_catch( 
      bind(throw_AX, _1),
      catch_exception<A1>( 
        var(ecount)++
      ),
      catch_exception<A2>( 
        var(ecount)++
      ),
      catch_exception<A3>( 
        var(ecount)++
      ),
      catch_exception<A4>( 
        var(ecount)++
      ),
      catch_exception<A5>( 
        var(ecount)++
      ),
      catch_all( 
        var(ecount)++
      )
    )(i);
  }
  BOOST_CHECK(ecount == 6);

  ecount = 0;
  for(int i=1; i<=7; i++) 
  {
    try_catch( 
      bind(throw_AX, _1),
      catch_exception<A1>( 
        var(ecount)++
      ),
      catch_exception<A2>( 
        var(ecount)++
      ),
      catch_exception<A3>( 
        var(ecount)++
      ),
      catch_exception<A4>( 
        var(ecount)++
      ),
      catch_exception<A5>( 
        var(ecount)++
      ),
      catch_exception<A6>( 
        var(ecount)++
      ),
      catch_all( 
        var(ecount)++
      )
    )(i);
  }
  BOOST_CHECK(ecount == 7);

  ecount = 0;
  for(int i=1; i<=8; i++) 
  {
    try_catch( 
      bind(throw_AX, _1),
      catch_exception<A1>( 
        var(ecount)++
      ),
      catch_exception<A2>( 
        var(ecount)++
      ),
      catch_exception<A3>( 
        var(ecount)++
      ),
      catch_exception<A4>( 
        var(ecount)++
      ),
      catch_exception<A5>( 
        var(ecount)++
      ),
      catch_exception<A6>( 
        var(ecount)++
      ),
      catch_exception<A7>( 
        var(ecount)++
      ),
      catch_all( 
        var(ecount)++
      )
    )(i);
  }
  BOOST_CHECK(ecount == 8);

  ecount = 0;
  for(int i=1; i<=9; i++) 
  {
    try_catch( 
      bind(throw_AX, _1),
      catch_exception<A1>( 
        var(ecount)++
      ),
      catch_exception<A2>( 
        var(ecount)++
      ),
      catch_exception<A3>( 
        var(ecount)++
      ),
      catch_exception<A4>( 
        var(ecount)++
      ),
      catch_exception<A5>( 
        var(ecount)++
      ),
      catch_exception<A6>( 
        var(ecount)++
      ),
      catch_exception<A7>( 
        var(ecount)++
      ),
      catch_exception<A8>( 
        var(ecount)++
      ),
      catch_all( 
        var(ecount)++
      )
    )(i);
  }
  BOOST_CHECK(ecount == 9);
}

void test_empty_catch_blocks() {
  try_catch(
    bind(throw_AX, _1), 
    catch_exception<A1>()
  )(make_const(1));

  try_catch(
    bind(throw_AX, _1), 
    catch_all()
  )(make_const(1));

}


void return_type_matching() {

//   Rules for return types of the lambda functors in try and catch parts:
// 1. The try part dictates the return type of the whole 
//    try_catch lambda functor
// 2. If return type of try part is void, catch parts can return anything, 
//    but the return types are ignored
// 3. If the return type of the try part is A, then each catch return type
//    must be implicitly convertible to A, or then it must throw for sure


  int i = 1;
  
  BOOST_CHECK(
    
    try_catch(
      _1 + 1,
      catch_exception<int>((&_1, rethrow())), // no match, but ok since throws
      catch_exception<char>(_e) // ok, char convertible to int 
    )(i)
 
    == 2
  );
  
  // note that while e.g. char is convertible to int, it is not convertible
  // to int&, (some lambda functors return references)
 
  //   try_catch(
  //     _1 += 1,
  //     catch_exception<char>(_e) // NOT ok, char not convertible to int& 
  //   )(i);

  // if you don't care about the return type, you can use make_void
  try_catch(
    make_void(_1 += 1),
    catch_exception<char>(_e) // since try is void, catch can return anything 
  )(i);
  BOOST_CHECK(i == 2);
  
  try_catch(
    (_1 += 1, throw_exception('a')),
    catch_exception<char>(_e) // since try throws, it is void, 
                              // so catch can return anything 
  )(i);
  BOOST_CHECK(i == 3);

  char a = 'a';
  try_catch(
    try_catch(
      throw_exception(1),
      catch_exception<int>(throw_exception('b'))
    ),
    catch_exception<char>( _1 = _e ) 
  )(a);
  BOOST_CHECK(a == 'b');
}
  
int test_main(int, char *[]) {   

  try 
  {
    test_different_number_of_catch_blocks();
    return_type_matching();
    test_empty_catch_blocks();
  }
  catch (int)
  {
    BOOST_CHECK(false);
  }
  catch(...)
  { 
    BOOST_CHECK(false);
  }


  return EXIT_SUCCESS;
}




