//  (C) Copyright Jeremy Siek 1999.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Revision History
//  22 Nov 2002 Thomas Witt
//       Added interoperability check.
//  08 Mar 2001   Jeremy Siek
//       Moved test of indirect iterator into its own file. It to
//       to be in iterator_adaptor_test.cpp.

#include <boost/config.hpp>
#include <iostream>
#include <algorithm>

#include <boost/iterator/indirect_iterator.hpp>
#include <boost/iterator/iterator_concepts.hpp>
#include <boost/iterator/new_iterator_tests.hpp>

#include <boost/detail/workaround.hpp>

#include <boost/concept_archetype.hpp>
#include <boost/concept_check.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

#include <boost/mpl/has_xxx.hpp>

#include <boost/type_traits/broken_compiler_spec.hpp>

#include <boost/detail/lightweight_test.hpp>

#include <vector>
#include <stdlib.h>
#include <set>

#if !defined(__SGI_STL_PORT)                            \
    && (defined(BOOST_MSVC_STD_ITERATOR)                \
        || BOOST_WORKAROUND(_CPPLIB_VER, <= 310)        \
        || BOOST_WORKAROUND(__GNUC__, <= 2))

// std container random-access iterators don't support mutable/const
// interoperability (but may support const/mutable interop).
# define NO_MUTABLE_CONST_STD_SET_ITERATOR_INTEROPERABILITY

#endif 


template <class T> struct see_type;
template <int I> struct see_val;

struct my_iterator_tag : public std::random_access_iterator_tag { };

using boost::dummyT;
BOOST_TT_BROKEN_COMPILER_SPEC(boost::shared_ptr<dummyT>)
    
typedef std::vector<int> storage;
typedef std::vector<int*> pointer_ra_container;
typedef std::set<storage::iterator> iterator_set;

template <class Container>
struct indirect_iterator_pair_generator
{
    typedef boost::indirect_iterator<typename Container::iterator> iterator;
    
    typedef boost::indirect_iterator<
        typename Container::iterator
      , typename iterator::value_type const
    > const_iterator;
};

void more_indirect_iterator_tests()
{
    storage store(1000);
    std::generate(store.begin(), store.end(), rand);
    
    pointer_ra_container ptr_ra_container;
    iterator_set iter_set;

    for (storage::iterator p = store.begin(); p != store.end(); ++p)
    {
        ptr_ra_container.push_back(&*p);
        iter_set.insert(p);
    }

    typedef indirect_iterator_pair_generator<pointer_ra_container> indirect_ra_container;

    indirect_ra_container::iterator db(ptr_ra_container.begin());
    indirect_ra_container::iterator de(ptr_ra_container.end());
    BOOST_TEST(static_cast<std::size_t>(de - db) == store.size());
    BOOST_TEST(db + store.size() == de);
    indirect_ra_container::const_iterator dci = db;
    
    BOOST_TEST(dci == db);
    
#ifndef NO_MUTABLE_CONST_RA_ITERATOR_INTEROPERABILITY
    BOOST_TEST(db == dci);
#endif
    
    BOOST_TEST(dci != de);
    BOOST_TEST(dci < de);
    BOOST_TEST(dci <= de);
    
#ifndef NO_MUTABLE_CONST_RA_ITERATOR_INTEROPERABILITY
    BOOST_TEST(de >= dci);
    BOOST_TEST(de > dci);
#endif
    
    dci = de;
    BOOST_TEST(dci == de);

    boost::random_access_iterator_test(db + 1, store.size() - 1, boost::next(store.begin()));
    
    *db = 999;
    BOOST_TEST(store.front() == 999);

    // Borland C++ is getting very confused about the typedefs here
    typedef boost::indirect_iterator<iterator_set::iterator> indirect_set_iterator;
    typedef boost::indirect_iterator<
        iterator_set::iterator
      , iterator_set::iterator::value_type const
    > const_indirect_set_iterator;

    indirect_set_iterator sb(iter_set.begin());
    indirect_set_iterator se(iter_set.end());
    const_indirect_set_iterator sci(iter_set.begin());
    BOOST_TEST(sci == sb);
    
# ifndef NO_MUTABLE_CONST_STD_SET_ITERATOR_INTEROPERABILITY
    BOOST_TEST(se != sci);
# endif
    
    BOOST_TEST(sci != se);
    sci = se;
    BOOST_TEST(sci == se);
    
    *boost::prior(se) = 888;
    BOOST_TEST(store.back() == 888);
    BOOST_TEST(std::equal(sb, se, store.begin()));

    boost::bidirectional_iterator_test(boost::next(sb), store[1], store[2]);
    BOOST_TEST(std::equal(db, de, store.begin()));
}

// element_type detector; defaults to true so the test passes when
// has_xxx isn't implemented
BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF(has_element_type, element_type, true)
    
int
main()
{
  dummyT array[] = { dummyT(0), dummyT(1), dummyT(2), 
                     dummyT(3), dummyT(4), dummyT(5) };
  const int N = sizeof(array)/sizeof(dummyT);

# if BOOST_WORKAROUND(BOOST_MSVC, == 1200)
  boost::shared_ptr<dummyT> zz((dummyT*)0);  // Why? I don't know, but it suppresses a bad instantiation.
# endif
  
  typedef std::vector<boost::shared_ptr<dummyT> > shared_t;
  shared_t shared;
  
  // Concept checks
  {
    typedef boost::indirect_iterator<shared_t::iterator> iter_t;

    BOOST_STATIC_ASSERT(
        has_element_type<
            boost::detail::iterator_traits<shared_t::iterator>::value_type
        >::value
        );
    
    typedef boost::indirect_iterator<
        shared_t::iterator
      , boost::iterator_value<shared_t::iterator>::type const
    > c_iter_t;

# ifndef NO_MUTABLE_CONST_RA_ITERATOR_INTEROPERABILITY
    boost::function_requires< boost_concepts::InteroperableIteratorConcept<iter_t, c_iter_t> >();
# endif 
  }

  // Test indirect_iterator_generator
  {
      for (int jj = 0; jj < N; ++jj)
          shared.push_back(boost::shared_ptr<dummyT>(new dummyT(jj)));
      
      dummyT* ptr[N];
      for (int k = 0; k < N; ++k)
          ptr[k] = array + k;

      typedef boost::indirect_iterator<dummyT**> indirect_iterator;

      typedef boost::indirect_iterator<dummyT**, dummyT const>
          const_indirect_iterator;

      indirect_iterator i(ptr);
      boost::random_access_iterator_test(i, N, array);

      boost::random_access_iterator_test(
          boost::indirect_iterator<shared_t::iterator>(shared.begin())
          , N, array);

      boost::random_access_iterator_test(boost::make_indirect_iterator(ptr), N, array);
    
      // check operator->
      assert((*i).m_x == i->foo());

      const_indirect_iterator j(ptr);
      boost::random_access_iterator_test(j, N, array);
    
      dummyT const*const* const_ptr = ptr;
      boost::random_access_iterator_test(boost::make_indirect_iterator(const_ptr), N, array);
      
      boost::const_nonconst_iterator_test(i, ++j);

      more_indirect_iterator_tests();
  }
  return boost::report_errors();
}
