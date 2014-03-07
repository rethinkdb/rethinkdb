#ifndef BOOST_STATECHART_EXAMPLE_UNIQUE_OBJECT_ALLOCATOR_HPP_INCLUDED
#define BOOST_STATECHART_EXAMPLE_UNIQUE_OBJECT_ALLOCATOR_HPP_INCLUDED
//////////////////////////////////////////////////////////////////////////////
// Copyright 2002-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/statechart/detail/avoid_unused_warning.hpp>

#include <boost/config.hpp>

#ifdef BOOST_MSVC
#  pragma warning( push )
#  pragma warning( disable: 4511 ) // copy constructor could not be generated
#  pragma warning( disable: 4512 ) // assignment operator could not be generated
#endif

#include <boost/type_traits/alignment_of.hpp>

#ifdef BOOST_MSVC
#  pragma warning( pop )
#endif

#include <boost/type_traits/type_with_alignment.hpp>
#include <boost/assert.hpp>

#include <cstddef> // size_t



//////////////////////////////////////////////////////////////////////////////
template< class T >
class UniqueObjectAllocator
{
  public:
    //////////////////////////////////////////////////////////////////////////
    static void * allocate( std::size_t size )
    {
      boost::statechart::detail::avoid_unused_warning( size );
      BOOST_ASSERT( !constructed_ && ( size == sizeof( T ) ) );
      constructed_ = true;
      return &storage_.data_[ 0 ];
    }

    static void deallocate( void * p, std::size_t size )
    {
      boost::statechart::detail::avoid_unused_warning( p );
      boost::statechart::detail::avoid_unused_warning( size );
      BOOST_ASSERT( constructed_ &&
        ( p == &storage_.data_[ 0 ] ) && ( size == sizeof( T ) ) );
      constructed_ = false;
    }

  private:
    //////////////////////////////////////////////////////////////////////////
    union storage
    {
      char data_[ sizeof( T ) ];
      typename boost::type_with_alignment<
        boost::alignment_of< T >::value >::type aligner_;
    };

    static bool constructed_;
    static storage storage_;
};

template< class T >
bool UniqueObjectAllocator< T >::constructed_ = false;
template< class T >
typename UniqueObjectAllocator< T >::storage
  UniqueObjectAllocator< T >::storage_;



#endif
