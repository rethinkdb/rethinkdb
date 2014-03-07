//  noncopyable.cpp
///
//  (C) Copyright Eric Niebler 2004.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/*
 Revision history:
   21 December 2005 : Initial version.
*/

#include <vector>
#include <boost/foreach.hpp>
#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>

struct noncopy_vector
  : std::vector<int>
  , private boost::noncopyable
{
  noncopy_vector() { }
};

struct noncopy_range
  : boost::iterator_range<noncopy_vector::iterator>
  , private boost::noncopyable
{
  noncopy_range() { }
};

// Tell FOREACH that noncopy_vector and noncopy_range are non-copyable.
// NOTE: this is only necessary if
//   a) your type does not inherit from boost::noncopyable, OR
//   b) Boost.Config defines BOOST_BROKEN_IS_BASE_AND_DERIVED for your compiler
#ifdef BOOST_BROKEN_IS_BASE_AND_DERIVED
inline boost::mpl::true_ *boost_foreach_is_noncopyable(noncopy_vector *&, boost::foreach::tag)
{
    return 0;
}

inline boost::mpl::true_ *boost_foreach_is_noncopyable(noncopy_range *&, boost::foreach::tag)
{
    return 0;
}
#endif

// tell FOREACH that noncopy_range is a lightweight proxy object
inline boost::mpl::true_ *boost_foreach_is_lightweight_proxy(noncopy_range *&, boost::foreach::tag)
{
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// main
//   
int main( int, char*[] )
{
    noncopy_vector v1;
    BOOST_FOREACH( int & i, v1 ) { (void)i; }

    noncopy_vector const v2;
    BOOST_FOREACH( int const & j, v2 ) { (void)j; }

    noncopy_range rng1;
    BOOST_FOREACH( int & k, rng1 ) { (void)k; }

    noncopy_range const rng2;
    BOOST_FOREACH( int & l, rng2 ) { (void)l; }

    return 0;
}
