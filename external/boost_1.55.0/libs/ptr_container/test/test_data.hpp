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
 
#include <boost/static_assert.hpp>
#include <boost/type_traits.hpp>
#include <boost/config.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/functional/hash.hpp>
#include <algorithm>
#include <iostream>
#include <string>
#include <utility>
#include <cstdlib>

using namespace std;
using namespace boost;

//////////////////////////////////////////////////////////////////////////////
// Test class 1: a class hierarchy
//////////////////////////////////////////////////////////////////////////////

class Base
{
    Base( const Base& r ) : data1(r.data1), data2(r.data2), 
        data3(r.data3), data(r.data) 
    { 
#ifdef PTR_CONTAINER_DEBUG
        objects++;
        std::cout <<"+ " << objects << "\n"; 
#endif
    }
    
    Base& operator=( const Base& );

public: // for test reasons only
    int data1, data2, data3;
    string data;
    
public:
    
    Base() : data1(1), data2(2), data3(rand()%256), 
             data(lexical_cast<string>(rand())) 
    {
#ifdef PTR_CONTAINER_DEBUG
        objects++;
        std::cout <<"+ " << objects << "\n"; 
#endif
    }
    
    virtual ~Base()                       
    {
#ifdef PTR_CONTAINER_DEBUG
        objects--;
        std::cout <<"- " << objects << "\n"; 
        if( objects < 0 )
            terminate();
#endif
    }
    
    void     print( ostream& out ) const  { do_print( out); }
    Base*    clone() const                { return do_clone(); }
    void     foo()                        { do_foo(); }
    
    virtual bool less_than( const Base& b ) const
    {
        return data3 < b.data3;
    }
    
    virtual bool equal( const Base& b ) const
    {
        return data1 == b.data1 && 
               data2 == b.data2 &&
               data3 == b.data3 &&
               data  == b.data;
    }

#ifdef PTR_CONTAINER_DEBUG
    static int objects;
#endif    
    
private:
    virtual void  do_print( ostream& /*out*/ ) const   { };
    virtual Base* do_clone() const                     { return new Base( *this ); }; 
    virtual void  do_foo()                             { };
};

#ifdef PTR_CONTAINER_DEBUG
int Base::objects = 0;
#endif



ostream& operator<<( ostream& out, const Base& b )
{
    b.print( out );
    return out;
}


//
//  We rely on argument dependent lookup
//  for this to be found
//
inline Base* new_clone( const Base& b )
{
    return b.clone();
}



inline bool operator<( const Base& l, const Base& r )
{
    return l.less_than( r );
}



inline bool operator>( const Base& l, const Base& r )
{
    return r < l;
}



inline bool operator==( const Base& l, const Base& r )
{
    return l.equal( r );
}



inline bool operator!=( const Base& l, const Base& r )
{
    return !l.equal( r );
}



inline std::size_t hash_value( const Base& b )
{
    std::size_t seed = 0;
    boost::hash_combine( seed, b.data );
    boost::hash_combine( seed, b.data1 );
    boost::hash_combine( seed, b.data2 );
    boost::hash_combine( seed, b.data3 );
    return seed;
}


class Derived_class : public Base
{   
public: // for test reasons only
    int i_;

private:
        
    virtual void  do_print( ostream& out ) const
    {
        out << i_;
    }
    
    
    virtual Base* do_clone() const
    {
        return new Derived_class;
    }
    
    virtual void do_foo() 
    {
        ++i_;
    }
    
public:
    Derived_class() : i_( rand() )
    { }

    virtual bool less_than( const Base& b ) const
    {
        const Derived_class& d = dynamic_cast<const Derived_class&>( b );
        return i_ < d.i_;
    }
};



inline std::size_t hash_value( const Derived_class& b )
{
    std::size_t seed = hash_value( static_cast<const Base&>( b ) );
    boost::hash_combine( seed, b.i_ );
    return seed;
}

//////////////////////////////////////////////////////////////////////////////
// Test class 2: a value class 
//////////////////////////////////////////////////////////////////////////////

class Value 
{   
public: // for test reasons only    
    string s_;
    
public:
    
    Value() : s_( boost::lexical_cast<string>( rand() ) ) 
    {}
    
    ~Value()      { /** debug code here */ }
    
    string name() const
    {
        return s_;
    }
};



inline bool operator<( const Value& l, const Value& r )
{
    return l.name() < r.name();
}



inline bool operator>( const Value& l, const Value& r )
{
    return l.name() > r.name();
}



inline bool operator==( const Value& l, const Value& r )
{
    return l.name() == r.name();
}



inline bool operator!=( const Value& l, const Value& r )
{
    return l.name() != r.name();
}



inline ostream& operator<<( ostream& out, const Value& v )
{
    return out << v.name() << " ";
}



inline std::size_t hash_value( const Value& v )
{
    return boost::hash_value( v.s_ );
}

//
// used to hide "unused variable" warnings
//
template< class T >
inline void hide_warning( T& /*r*/ )
{ }

//
// used to customize tests for circular_buffer
//
template< class Cont >
struct set_capacity
{
    void operator()( Cont& ) const
    { }
};

//
//  transfer() test
// 

template< class Cont1, class Cont2 >
void transfer_test( Cont1& from, Cont2& to )
{
    BOOST_MESSAGE( "starting container transfer test" );
    BOOST_CHECK( !from.empty() );
    to. BOOST_NESTED_TEMPLATE transfer<Cont1>( from );
    BOOST_CHECK( !to.empty() );
    BOOST_MESSAGE( "finishing container transfer test" );
}


//
// test of copy operations
//

template< class BaseContainer, class DerivedContainer, class Derived >
void container_assignment_test()
{
    BOOST_MESSAGE( "starting container assignment test" );

    DerivedContainer derived;
    set_capacity<DerivedContainer>()( derived );
    derived.insert( derived.begin(), new Derived );
    derived.insert( derived.begin(), new Derived );

    BaseContainer base( derived );
    BOOST_CHECK_EQUAL( derived.size(), base.size() );
    base.clear();
    base = derived;
    BOOST_CHECK_EQUAL( derived.size(), base.size() );
    BaseContainer base2( base );
    BOOST_CHECK_EQUAL( base2.size(), base.size() );
    base2 = base;
    BOOST_CHECK_EQUAL( base2.size(), base.size() );
    base = base;

    BOOST_MESSAGE( "finished container assignment test" );
}


