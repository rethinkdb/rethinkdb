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
#include <boost/utility.hpp>
#include <algorithm>
#include <iostream>

using namespace std;
using namespace boost;

//
// Forward declare 'allocate_clone()' to be able
// use clonability of 'Composite' inline in the class.
// This is normally not needed when using .hpp + .cpp files.
//
class Composite;
Composite* new_clone( const Composite& );


class Composite 
{
    typedef ptr_vector<Composite>         composite_t;
    typedef composite_t::iterator         iterator;
    typedef composite_t::const_iterator   const_iterator;
    typedef composite_t::size_type        size_type; 
    composite_t                           elements_;

    //
    // only used internally for 'clone()'
    //
    Composite( const Composite& r ) : elements_( r.elements_.clone() )
    { }
    
    //
    // this class is not Copyable nor Assignable 
    //
    void operator=( const Composite& );

public:
    Composite()
    { }
    
    //
    // of course detructor is virtual
    //
    virtual ~Composite()
    { }
 
    //
    // one way of adding new elements
    //
    void add( Composite* c )
    {
        elements_.push_back( c );
    }
    
    //
    // second way of adding new elements
    //
    void add( Composite& c )
    {
        elements_.push_back( new_clone( c ) );
    }

    void remove( iterator where )
    {
        elements_.erase( where );
    }
  
    //
    // recusively count the elements 
    //
    size_type size() const
    {
        size_type res = 0;
        for( const_iterator i = elements_.begin(); i != elements_.end(); ++i )
            res += i->size();
        return 1 /* this */ + res;
    }
    
    void foo()
    {
        do_foo();
        for( iterator i = elements_.begin(); i != elements_.end(); ++i )
            i->foo();
    }
    
    //
    // this class is clonable and this is the callback for 'allocate_clone()'
    //
    Composite* clone() const
    {
        return do_clone();
    }
    
private:
    virtual void do_foo() 
    {
        cout << "composite base" << "\n";
    }

    virtual Composite* do_clone() const
    {
        return new Composite( *this );
    }
};

//
// make 'Composite' clonable; note that we do not need to overload
// the function in the 'boost' namespace.
//
Composite* new_clone( const Composite& c )
{
    return c.clone();
}


class ConcreteComposite1 : public Composite
{
    virtual void do_foo() 
    {
        cout << "composite 1" << "\n"; 
    }
  
    virtual Composite* do_clone() const
    {
        return new ConcreteComposite1();
    }
};


class ConcreteComposite2 : public Composite
{
    virtual void do_foo() 
    {
        cout << "composite 2" << "\n";
    }
  
    virtual Composite* do_clone() const
    {
        return new ConcreteComposite2();
    }
};

void test_incomplete()
{
    Composite c;
    c.add( new ConcreteComposite1 );
    c.add( new ConcreteComposite2 ); 
    BOOST_CHECK_EQUAL( c.size(), 3u );
    c.add( new_clone( c ) ); // add c to itself
    BOOST_CHECK_EQUAL( c.size(), 6u );
    c.add( c );              // add c to itself
    BOOST_CHECK_EQUAL( c.size(), 12u );
    c.foo();     
}

using namespace boost;


#include <boost/test/unit_test.hpp>
using boost::unit_test::test_suite;

test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite* test = BOOST_TEST_SUITE( "Pointer Container Test Suite" );

    test->add( BOOST_TEST_CASE( &test_incomplete ) );

    return test;
}

//
// todo: remake example with shared_ptr
//

