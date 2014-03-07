#ifndef BOOST_STATECHART_EXAMPLE_UNIQUE_OBJECT_HPP_INCLUDED
#define BOOST_STATECHART_EXAMPLE_UNIQUE_OBJECT_HPP_INCLUDED
//////////////////////////////////////////////////////////////////////////////
// Copyright 2002-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include "UniqueObjectAllocator.hpp"

#include <cstddef> // size_t



//////////////////////////////////////////////////////////////////////////////
template< class Derived >
class UniqueObject
{
  public:
    //////////////////////////////////////////////////////////////////////////
    void * operator new( std::size_t size )
    {
      return UniqueObjectAllocator< Derived >::allocate( size );
    }

    void operator delete( void * p, std::size_t size )
    {
      UniqueObjectAllocator< Derived >::deallocate( p, size );
    }

  protected:
    //////////////////////////////////////////////////////////////////////////
    UniqueObject() {}
    ~UniqueObject() {}
};



#endif
