// (C) Copyright Jeremy Siek 2000.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/concept_check.hpp>
#include <boost/concept_archetype.hpp>

/*

  This file verifies that function_requires() of the Boost Concept
  Checking Library does not cause errors when it is not suppose to
  and verifies that the concept archetypes meet the requirements of
  their matching concepts.

*/


int
main()
{
  using namespace boost;

  //===========================================================================
  // Basic Concepts
  {
    typedef default_constructible_archetype<> foo;
    function_requires< DefaultConstructible<foo> >();
  }
  {
    typedef assignable_archetype<> foo;
    function_requires< Assignable<foo> >();
  }
  {
    typedef copy_constructible_archetype<> foo;
    function_requires< CopyConstructible<foo> >();
  }
  {
    typedef sgi_assignable_archetype<> foo;
    function_requires< SGIAssignable<foo> >();
  }
  {
    typedef copy_constructible_archetype<> foo;
    typedef convertible_to_archetype<foo> convertible_to_foo;
    function_requires< Convertible<convertible_to_foo, foo> >();
  }
  {
    function_requires< Convertible<boolean_archetype, bool> >();
  }
  {
    typedef equality_comparable_archetype<> foo;
    function_requires< EqualityComparable<foo> >();
  }
  {
    typedef less_than_comparable_archetype<> foo;
    function_requires< LessThanComparable<foo> >();
  }
  {
    typedef comparable_archetype<> foo;
    function_requires< Comparable<foo> >();
  }
  {
    typedef equal_op_first_archetype<> First;
    typedef equal_op_second_archetype<> Second;
    function_requires< EqualOp<First, Second> >();
  }
  {
    typedef not_equal_op_first_archetype<> First;
    typedef not_equal_op_second_archetype<> Second;
    function_requires< NotEqualOp<First, Second> >();
  }
  {
    typedef less_than_op_first_archetype<> First;
    typedef less_than_op_second_archetype<> Second;
    function_requires< LessThanOp<First, Second> >();
  }
  {
    typedef less_equal_op_first_archetype<> First;
    typedef less_equal_op_second_archetype<> Second;
    function_requires< LessEqualOp<First, Second> >();
  }
  {
    typedef greater_than_op_first_archetype<> First;
    typedef greater_than_op_second_archetype<> Second;
    function_requires< GreaterThanOp<First, Second> >();
  }
  {
    typedef greater_equal_op_first_archetype<> First;
    typedef greater_equal_op_second_archetype<> Second;
    function_requires< GreaterEqualOp<First, Second> >();
  }

  {
    typedef copy_constructible_archetype<> Return;
    typedef plus_op_first_archetype<Return> First;
    typedef plus_op_second_archetype<Return> Second;
    function_requires< PlusOp<Return, First, Second> >();
  }

  //===========================================================================
  // Function Object Concepts

  {
    typedef generator_archetype<null_archetype<> > foo;
    function_requires< Generator<foo, null_archetype<> > >();
  }
#if !defined BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
  {
    function_requires< Generator< void_generator_archetype, void > >();
  }
#endif
  {
    typedef unary_function_archetype<int, int> F;
    function_requires< UnaryFunction<F, int, int> >();
  }
  {
    typedef binary_function_archetype<int, int, int> F;
    function_requires< BinaryFunction<F, int, int, int> >();
  }
  {
    typedef unary_predicate_archetype<int> F;
    function_requires< UnaryPredicate<F, int> >();
  }
  {
    typedef binary_predicate_archetype<int, int> F;
    function_requires< BinaryPredicate<F, int, int> >();
  }

  //===========================================================================
  // Iterator Concepts
  {
    typedef input_iterator_archetype<null_archetype<> > Iter;
    function_requires< InputIterator<Iter> >();
  }
  {
    typedef output_iterator_archetype<int> Iter;
    function_requires< OutputIterator<Iter, int> >();
  }
  {
    typedef input_output_iterator_archetype<int> Iter;
    function_requires< InputIterator<Iter> >();
    function_requires< OutputIterator<Iter, int> >();
  }
  {
    typedef forward_iterator_archetype<null_archetype<> > Iter;
    function_requires< ForwardIterator<Iter> >();
  }
  {
    typedef mutable_forward_iterator_archetype<assignable_archetype<> > Iter;
    function_requires< Mutable_ForwardIterator<Iter> >();
  }
  {
    typedef bidirectional_iterator_archetype<null_archetype<> > Iter;
    function_requires< BidirectionalIterator<Iter> >();
  }
  {
    typedef mutable_bidirectional_iterator_archetype<assignable_archetype<> > 
      Iter;
    function_requires< Mutable_BidirectionalIterator<Iter> >();
  }
  {
    typedef random_access_iterator_archetype<null_archetype<> > Iter;
    function_requires< RandomAccessIterator<Iter> >();
  }
  {
    typedef mutable_random_access_iterator_archetype<assignable_archetype<> > 
      Iter;
    function_requires< Mutable_RandomAccessIterator<Iter> >();
  }

  //===========================================================================
  // Container Concepts

  // UNDER CONSTRUCTION

  return 0;
}
