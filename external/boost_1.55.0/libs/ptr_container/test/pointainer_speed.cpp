//
// Boost.Pointer Container
//
//  Copyright Thorsten Ottosen 2003-2005. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/ptr_container/
//

#include "test_data.hpp"
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/progress.hpp>


using namespace boost;
using namespace std;


typedef shared_ptr<Base> PolyPtr;

struct PolyPtrOps
{
  void operator()( const PolyPtr& a )
    { a->foo(); }
};

struct less_than
{
    bool operator()( const PolyPtr& l, const PolyPtr& r ) const
    {
        return *l < *r;
    }

    bool operator()( const Base* l, const Base* r ) const
    {
        return *l < *r;
    }
};

struct greater_than
{
    bool operator()( const PolyPtr& l, const PolyPtr& r ) const
    {
        return *l > *r;
    }

    bool operator()( const Base* l, const Base* r ) const
    {
        return *l > *r;
    }
};

struct data_less_than
{
    bool operator()( const PolyPtr& l, const PolyPtr& r ) const
    {
        return l->data_less_than(*r);
    }

    bool operator()( const Base* l, const Base* r ) const
    {
        return l->data_less_than(*r);
    }
};

struct data_less_than2
{
    bool operator()( const PolyPtr& l, const PolyPtr& r ) const
    {
        return l->data_less_than2(*r);
    }

    bool operator()( const Base* l, const Base* r ) const
    {
        return l->data_less_than2(*r);
    }
};


void test_speed()
{
    enum { size = 50000 };
    vector<PolyPtr>   svec;
    ptr_vector<Base>  pvec;
    
    {
        progress_timer timer;
        for( int i = 0; i < size; ++i )
            svec.push_back( PolyPtr( new Derived ) );
        cout << "\n shared_ptr call new: ";
    }

    {
        progress_timer timer;
        for( int i = 0; i < size; ++i )
            pvec.push_back( new Derived );        
        cout << "\n smart container call new: ";
    }
    
    {
        progress_timer timer;
        for_each( svec.begin(), svec.end(), PolyPtrOps() );
        cout << "\n shared_ptr call foo(): ";
    }
    
    {
        progress_timer timer;
        for_each( pvec.begin(), pvec.end(), mem_fun_ref( &Base::foo ) );
        cout << "\n smart container call foo(): ";
    }
        
    {
        progress_timer timer;
        sort( svec.begin(), svec.end(), less_than() );
        cout << "\n shared_ptr call sort(): "; 
    }

    {
        progress_timer timer;
        sort( pvec.ptr_begin(), pvec.ptr_end(), less_than() );
        cout << "\n smart container call sort(): "; 
    }

    {
        progress_timer timer;
        sort( svec.begin(), svec.end(), greater_than() );
        cout << "\n shared_ptr call sort() #2: "; 
    }

    {
        progress_timer timer;
        sort( pvec.ptr_begin(), pvec.ptr_end(), greater_than() );
        cout << "\n smart container call sort() #2: "; 
    }

    {
        progress_timer timer;
        sort( svec.begin(), svec.end(), data_less_than() );
        cout << "\n shared_ptr call sort() #3: "; 
    }

    {
        progress_timer timer;
        sort( pvec.ptr_begin(), pvec.ptr_end(), data_less_than() );
        cout << "\n smart container call sort() #3: "; 
    }

    {
        progress_timer timer;
        sort( svec.begin(), svec.end(), data_less_than2() );
        cout << "\n shared_ptr call sort() #4: "; 
    }

    {
        progress_timer timer;
        sort( pvec.ptr_begin(), pvec.ptr_end(), data_less_than2() );
        cout << "\n smart container call sort() #4: "; 
    }
        
     vector<Base*> copy1;
     for( ptr_vector<Base>::ptr_iterator i = pvec.ptr_begin(); i != pvec.ptr_end(); ++ i )
         copy1.push_back( *i ); 
     
     sort( pvec.ptr_begin(), pvec.ptr_end() );

    
    vector<Base*> copy2;
    for( ptr_vector<Base>::ptr_iterator i = pvec.ptr_begin(); i != pvec.ptr_end(); ++ i )
        copy2.push_back( *i ); 
   
    
    for( unsigned int i =  0; i < copy1.size(); ++i )
    {
        bool found = false;
        for( int j = 0; j < copy1.size(); ++ j )
            if( copy1[i] == copy2[j] )
                found = true;

        if( !found )
            cout << copy1[i] << endl;
    }   

    BOOST_REQUIRE( pvec.size() == size );
    cout << endl;
}


#include <boost/test/unit_test.hpp>
using boost::unit_test::test_suite;

test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite* test = BOOST_TEST_SUITE( "Pointer Container Test Suite" );

    test->add( BOOST_TEST_CASE( &test_speed ) );

    return test;
}




