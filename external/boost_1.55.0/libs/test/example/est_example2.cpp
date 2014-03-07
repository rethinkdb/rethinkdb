//  (C) Copyright Gennadiy Rozental 2005-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

// Boost.Test
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/test/exception_safety.hpp>
#include <boost/test/mock_object.hpp>
using namespace boost::itest;

// Boost
#include <boost/bind.hpp>

//____________________________________________________________________________//

// Here is an example of simple (incorrect) stack implementation

template<class T>
class stack {
public:
    explicit stack( int init_capacity = 10 )
    : m_capacity( init_capacity )
    , m_size( 0 )
    , m_v( BOOST_ITEST_NEW(T)[m_capacity] )
    {
        BOOST_ITEST_SCOPE( stack::stack );
    }
    ~stack()
    {
        delete[] m_v;
    }

    void push( T const& element )
    {
        BOOST_ITEST_SCOPE( stack::push );

        if( m_size == m_capacity ) {
            m_capacity *= 2;
            T* new_buffer = BOOST_ITEST_NEW( T )[m_capacity];
            for( unsigned i = 0; i < m_size; i++ ) {
                new_buffer[i] = m_v[i];
            }
            delete [] m_v;
            m_v = new_buffer;
        }
        m_v[m_size++] = element;
    }
    unsigned size() { return m_size; }

private:
    unsigned    m_capacity;
    unsigned    m_size;
    T*          m_v;
};

//____________________________________________________________________________//

BOOST_TEST_EXCEPTION_SAFETY( test_stack_push )
{
    stack<mock_object<> > st( 2 );

    for( unsigned i = 0; i < 3; ++i ) {
        try {
            st.push( mock_object<>::prototype() );
        }
        catch( ... ) {
            // this invariant checks that in case of failed push number of elements doesn't change
            BOOST_CHECK_EQUAL( i, st.size() );
            throw;
        }
    }
}

//____________________________________________________________________________//

// EOF
