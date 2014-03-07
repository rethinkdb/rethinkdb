// Copyright 2008 Gunter Winkler <guwi17@gmx.de>
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _HPP_TESTHELPER_
#define _HPP_TESTHELPER_

#include <utility>

static unsigned _success_counter = 0;
static unsigned _fail_counter    = 0;

static inline
void assertTrue(const char* message, bool condition) {
#ifndef NOMESSAGES
  std::cout << message;
#endif
  if ( condition ) {
    ++ _success_counter;
    std::cout << "1\n"; // success
  } else {
    ++ _fail_counter;
    std::cout << "0\n"; // failed
  }
}

template < class T >
void assertEquals(const char* message, T expected, T actual) {
#ifndef NOMESSAGES
  std::cout << message;
#endif
  if ( expected == actual ) {
    ++ _success_counter;
    std::cout << "1\n"; // success
  } else {
    #ifndef NOMESSAGES
      std::cout << " expected " << expected << " actual " << actual << " ";
    #endif
    ++ _fail_counter;
    std::cout << "0\n"; // failed
  }
}

static
std::pair<unsigned, unsigned> getResults() {
  return std::make_pair(_success_counter, _fail_counter);
}

template < class M1, class M2 >
bool compare( const boost::numeric::ublas::matrix_expression<M1> & m1, 
              const boost::numeric::ublas::matrix_expression<M2> & m2 ) {
  size_t size1 = (std::min)(m1().size1(), m2().size1());
  size_t size2 = (std::min)(m1().size2(), m2().size2());
  for (size_t i=0; i < size1; ++i) {
    for (size_t j=0; j < size2; ++j) {
      if ( m1()(i,j) != m2()(i,j) ) return false;
    }
  }
  return true;
}

template < class M1, class M2 >
bool compare( const boost::numeric::ublas::vector_expression<M1> & m1, 
              const boost::numeric::ublas::vector_expression<M2> & m2 ) {
  size_t size = (std::min)(m1().size(), m2().size());
  for (size_t i=0; i < size; ++i) {
    if ( m1()(i) != m2()(i) ) return false;
  }
  return true;
}

#endif
