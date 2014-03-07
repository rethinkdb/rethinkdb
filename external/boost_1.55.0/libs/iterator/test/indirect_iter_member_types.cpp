//  (C) Copyright Jeremy Siek 2004.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Revision History
//  03 Jan 2004   Jeremy Siek
//       First draft.


#include <boost/config.hpp>
#include <iostream>

#include <boost/iterator/indirect_iterator.hpp>
#include <boost/static_assert.hpp>
#include "static_assert_same.hpp"
#include <boost/type_traits/same_traits.hpp>

struct zow { };

struct my_ptr {
  typedef zow const element_type;
  zow const& operator*() const;
//  typedef const zow& reference;
//  typedef const zow* pointer;
//  typedef void difference_type;
//  typedef boost::no_traversal_tag iterator_category;
};

BOOST_TT_BROKEN_COMPILER_SPEC(my_ptr)
BOOST_TT_BROKEN_COMPILER_SPEC(zow)

// Borland 5.6.4 and earlier drop const all over the place, so this
// test will fail in the lines marked with (**)
    
int main()
{
  {
    typedef boost::indirect_iterator<int**> Iter;
    STATIC_ASSERT_SAME(Iter::value_type, int);
    STATIC_ASSERT_SAME(Iter::reference, int&);  
    STATIC_ASSERT_SAME(Iter::pointer, int*);  
    STATIC_ASSERT_SAME(Iter::difference_type, std::ptrdiff_t);
    
    BOOST_STATIC_ASSERT((boost::is_convertible<Iter::iterator_category,
             std::random_access_iterator_tag>::value));
    BOOST_STATIC_ASSERT((boost::is_convertible<boost::iterator_traversal<Iter>::type,
             boost::random_access_traversal_tag>::value));
  }
  {
    typedef boost::indirect_iterator<int const**> Iter;
    STATIC_ASSERT_SAME(Iter::value_type, int);
    STATIC_ASSERT_SAME(Iter::reference, const int&);
    STATIC_ASSERT_SAME(Iter::pointer, const int*);    // (**)
  }
  {
    typedef boost::indirect_iterator<int**, int> Iter;
    STATIC_ASSERT_SAME(Iter::value_type, int);
    STATIC_ASSERT_SAME(Iter::reference, int&);  
    STATIC_ASSERT_SAME(Iter::pointer, int*);  
  }
  {
    typedef boost::indirect_iterator<int**, const int> Iter;
    STATIC_ASSERT_SAME(Iter::value_type, int);
    STATIC_ASSERT_SAME(Iter::reference, const int&);  
    STATIC_ASSERT_SAME(Iter::pointer, const int*);    // (**)
  }
  {
    typedef boost::indirect_iterator<my_ptr*> Iter;
    STATIC_ASSERT_SAME(Iter::value_type, zow);
    STATIC_ASSERT_SAME(Iter::reference, const zow&);  // (**)
    STATIC_ASSERT_SAME(Iter::pointer, const zow*);    // (**)
    
    STATIC_ASSERT_SAME(Iter::difference_type, std::ptrdiff_t);
    
    BOOST_STATIC_ASSERT((boost::is_convertible<Iter::iterator_category,
             std::random_access_iterator_tag>::value));
    BOOST_STATIC_ASSERT((boost::is_convertible<boost::iterator_traversal<Iter>::type,
             boost::random_access_traversal_tag>::value));
  }
  {
    typedef boost::indirect_iterator<char**, int, std::random_access_iterator_tag, long&, short> Iter;
    STATIC_ASSERT_SAME(Iter::value_type, int);
    STATIC_ASSERT_SAME(Iter::reference, long&);  
    STATIC_ASSERT_SAME(Iter::pointer, int*);  
    STATIC_ASSERT_SAME(Iter::difference_type, short);  
  }
  return 0;
}
