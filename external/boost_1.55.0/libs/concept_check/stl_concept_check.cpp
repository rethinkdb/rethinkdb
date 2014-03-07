// (C) Copyright Jeremy Siek 2000.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//
// This file checks to see if various standard container
// implementations live up to requirements specified in the C++
// standard. As many implementations do not live to the requirements,
// it is not uncommon for this file to fail to compile. The
// BOOST_HIDE_EXPECTED_ERRORS macro is provided here if you want to
// see as much of this file compile as possible.
//

#include <boost/concept_check.hpp>

#include <iterator>
#include <set>
#include <map>
#include <vector>
#include <list>
#include <deque>
#if 0
#include <slist>
#endif

// Define this macro if you want to hide the expected error, that is,
// error in the various C++ standard library implementations.
//
//#define BOOST_HIDE_EXPECTED_ERRORS

int
main()
{
  using namespace boost;

#if defined(_ITERATOR_) && defined(BOOST_HIDE_EXPECTED_ERRORS)
  // VC++ STL implementation is not standard conformant and
  // fails to pass these concept checks
#else
  typedef std::vector<int> Vector;
  typedef std::deque<int> Deque;
  typedef std::list<int> List;

  // VC++ missing pointer and const_pointer typedefs
  function_requires< Mutable_RandomAccessContainer<Vector> >();
  function_requires< BackInsertionSequence<Vector> >();

#if !(defined(__GNUC__) && defined(BOOST_HIDE_EXPECTED_ERRORS))
#if !((defined(__sgi) || (defined(__DECCXX) && defined(_RWSTD_VER) && _RWSTD_VER <= 0x0203)) \
  && defined(BOOST_HIDE_EXPECTED_ERRORS))
  // old deque iterator missing n + iter operation
  function_requires< Mutable_RandomAccessContainer<Deque> >();
#endif
  // warnings about signed and unsigned in old deque version
  function_requires< FrontInsertionSequence<Deque> >();
  function_requires< BackInsertionSequence<Deque> >();
#endif

  // VC++ missing pointer and const_pointer typedefs
  function_requires< Mutable_ReversibleContainer<List> >();
  function_requires< FrontInsertionSequence<List> >();
  function_requires< BackInsertionSequence<List> >();

#if 0
  typedef BOOST_STD_EXTENSION_NAMESPACE::slist<int> SList;
  function_requires< FrontInsertionSequence<SList> >();
#endif

  typedef std::set<int> Set;
  typedef std::multiset<int> MultiSet;
  typedef std::map<int,int> Map;
  typedef std::multimap<int,int> MultiMap;

  function_requires< SortedAssociativeContainer<Set> >();
  function_requires< SimpleAssociativeContainer<Set> >();
  function_requires< UniqueAssociativeContainer<Set> >();

  function_requires< SortedAssociativeContainer<MultiSet> >();
  function_requires< SimpleAssociativeContainer<MultiSet> >();
  function_requires< MultipleAssociativeContainer<MultiSet> >();

  function_requires< SortedAssociativeContainer<Map> >();
  function_requires< UniqueAssociativeContainer<Map> >();
  function_requires< PairAssociativeContainer<Map> >();

  function_requires< SortedAssociativeContainer<MultiMap> >();
  function_requires< MultipleAssociativeContainer<MultiMap> >();
  function_requires< PairAssociativeContainer<MultiMap> >();
#endif

  return 0;
}
