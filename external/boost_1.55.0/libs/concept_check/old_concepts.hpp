// Copyright Jeremy Siek, David Abrahams 2000-2006. Distributed under
// the Boost Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_LIBS_CONCEPT_CHECK_OLD_CONCEPTS_DWA2006428_HPP
# define BOOST_LIBS_CONCEPT_CHECK_OLD_CONCEPTS_DWA2006428_HPP

#include <boost/concept_check.hpp>

namespace old
{
  template <class TT>
  void require_boolean_expr(const TT& t) {
    bool x = t;
    boost::ignore_unused_variable_warning(x);
  }
  
  template <class TT>
  struct EqualityComparableConcept
  {
    void constraints() {
        boost::require_boolean_expr(a == b);
        boost::require_boolean_expr(a != b);
    }
    TT a, b;
  };

  template <class Func, class Return, class Arg>
  struct UnaryFunctionConcept
  {
    // required in case any of our template args are const-qualified:
    UnaryFunctionConcept();
    
    void constraints() {
      r = f(arg); // require operator()
    }
    Func f;
    Arg arg;
    Return r;
  };

  template <class Func, class Return, class First, class Second>
  struct BinaryFunctionConcept
  {
    void constraints() { 
      r = f(first, second); // require operator()
    }
    Func f;
    First first;
    Second second;
    Return r;
  };

#define DEFINE_BINARY_PREDICATE_OP_CONSTRAINT(OP,NAME) \
  template <class First, class Second> \
  struct NAME { \
    void constraints() { (void)constraints_(); } \
    bool constraints_() {  \
      return  a OP b; \
    } \
    First a; \
    Second b; \
  }

  DEFINE_BINARY_PREDICATE_OP_CONSTRAINT(==, EqualOpConcept);
}

#endif // BOOST_LIBS_CONCEPT_CHECK_OLD_CONCEPTS_DWA2006428_HPP
