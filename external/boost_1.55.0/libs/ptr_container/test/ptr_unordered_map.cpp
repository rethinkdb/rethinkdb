//
// Boost.Pointer Container
//
//  Copyright Thorsten Ottosen 2008. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/ptr_container/
//

#include "test_data.hpp"
#include <boost/test/unit_test.hpp>
#include <boost/ptr_container/exception.hpp>
#include <boost/range/sub_range.hpp>
#include <boost/cast.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

//
// abstract base class definition
// 
struct abstract_base
{
    virtual ~abstract_base() {}
    virtual void foo() = 0;
    virtual abstract_base* clone() const = 0;
};

struct implementation : abstract_base
{
    implementation()
    { }

    implementation( const implementation& )
    { }
    
    implementation( int, std::string, int, std::string )
    { }

    virtual void foo() {}
    virtual abstract_base* clone() const
    {
        return new implementation( *this );
    }
};

inline std::ostream& operator<<( std::ostream& out, const abstract_base& r )
{
    return out;    
}

inline abstract_base* new_clone( const abstract_base& r )
{
    return r.clone();
}

inline std::size_t hash_value( const abstract_base& b )
{
    return boost::hash_value( &b );
}

//
// ptr_map test
// 

template< typename C, typename B, typename T >
void ptr_map_test();

template< class Key >
Key get_next_key( const Key& k );

template<>
int get_next_key<int>( const int& )
{
    return rand();
}

template<>
std::string get_next_key<std::string>( const std::string& )
{
    return boost::lexical_cast<std::string>( rand() );
}



template< typename C, typename B, typename T >
void ptr_map_test()
{
    using namespace boost;

    BOOST_MESSAGE( "starting associative container test" );
    enum { max_cnt = 10, size = 100 };
    C  c;
    BOOST_CHECK( c.size() == 0 );

    const C c2( c.begin(), c.end() );
    BOOST_CHECK( c.size() == c2.size() );

    C c3;

    BOOST_MESSAGE( "finished construction test" );

    BOOST_DEDUCED_TYPENAME C::allocator_type alloc        = c.get_allocator();
    BOOST_DEDUCED_TYPENAME C::iterator i                  = c.begin();
    BOOST_DEDUCED_TYPENAME C::const_iterator ci           = c2.begin();
    BOOST_DEDUCED_TYPENAME C::iterator i2                 = c.end();
    BOOST_DEDUCED_TYPENAME C::const_iterator ci2          = c2.begin();
    ci = c.cbegin();
    ci = c.cend();

    BOOST_DEDUCED_TYPENAME C::key_type a_key;

    BOOST_MESSAGE( "finished iterator test" );

    BOOST_DEDUCED_TYPENAME C::size_type s                 = c.size();
    BOOST_DEDUCED_TYPENAME C::size_type s2                = c.max_size();
    hide_warning(s2);
    BOOST_CHECK_EQUAL( c.size(), s );
    bool b                                                = c.empty();
    hide_warning(b);
    BOOST_MESSAGE( "finished accessors test" );

    a_key = get_next_key( a_key );
    c.insert( a_key, new T );
    a_key = get_next_key( a_key );
    c.insert( a_key, new T );
    c3.insert( c.begin(), c.end() );
    c.insert( c3 );
    c.erase( c.begin() );
    BOOST_CHECK( c3.end() == c3.erase( boost::make_iterator_range(c3) ) );
    c3.erase( a_key );

    BOOST_CHECK( c3.empty() );
    c.swap( c3 );
    swap(c,c3);
    swap(c3,c);
    BOOST_CHECK( !c3.empty() );
    c3.clear();
    BOOST_CHECK( c3.empty() );
    BOOST_MESSAGE( "finished modifiers test" );


    a_key = get_next_key( a_key );
    c.insert( a_key, new T );
    a_key = get_next_key( a_key );
    c.insert( a_key, std::auto_ptr<T>( new T ) );
    typename C::auto_type ptr2  = c.release( c.begin() );
    std::auto_ptr<C> ap         = c.release();
    c                           = c2.clone();
    BOOST_MESSAGE( "finished release/clone test" );


    a_key = get_next_key( a_key );
    c3.insert( a_key, new T );
    a_key = get_next_key( a_key );
    c3.insert( a_key, new T );

    c. BOOST_NESTED_TEMPLATE transfer<C>( c3.begin(), c3 );
    c. BOOST_NESTED_TEMPLATE transfer<C>( c3.begin(), c3.end(), c3 );
    BOOST_CHECK( c3.empty() );
    BOOST_CHECK( !c.empty() );
    c3. BOOST_NESTED_TEMPLATE transfer<C>( c );
    BOOST_CHECK( !c3.empty() );
    BOOST_CHECK( c.empty() );
#ifdef BOOST_NO_SFINAE
#else
    c. BOOST_NESTED_TEMPLATE transfer<C>( make_iterator_range(c3), c3 );
    BOOST_CHECK( !c.empty() );
    BOOST_CHECK( c3.empty() );
    c3. BOOST_NESTED_TEMPLATE transfer<C>(c);
#endif
    BOOST_MESSAGE( "finished transfer test" );

    BOOST_CHECK( !c3.empty() );
    c3.replace( c3.begin(), new T );
    c3.replace( c3.begin(), std::auto_ptr<T>( new T ) );
    BOOST_MESSAGE( "finished set/map interface test" );

    // @todo: make macro with algorithms so that the right erase() is called.
    //  c.unique();
    //  c.unique( std::not_equal_to<T>() );
    //  c.remove( T() );
    //  c.remove_if( std::binder1st< std::equal_to<T> >( T() ) );

    sub_range<C>        sub;
    sub_range<const C> csub;

    i  = c.find( get_next_key( a_key ) );
    ci = c2.find( get_next_key( a_key ) );
    c2.count( get_next_key( a_key ) );
    sub  = c.equal_range( get_next_key( a_key ) );
    csub = c2.equal_range( get_next_key( a_key ) );

    try
    {
        c.at( get_next_key( a_key ) );
    }
    catch( const bad_ptr_container_operation& )
    { }

    try
    {
        c2.at( get_next_key( a_key ) );
    }
    catch( const bad_ptr_container_operation& )
    { }

    BOOST_MESSAGE( "finished algorithms interface test" );

    typename C::iterator it = c.begin(), e = c.end();
    for( ; it != e; ++it )
    {
        std::cout << "\n mapped value = " << *it->second << " key = " << it->first;
        //std::cout << "\n mapped value = " << it.value() << " key = " << it.key();
    }
        
    BOOST_MESSAGE( "finished iterator test" );

    a_key = get_next_key( a_key );
    c.insert( a_key, new T );
    c.erase( a_key );
    c.erase( a_key );

}



template< class CDerived, class CBase, class T >
void test_transfer()
{
    CDerived from;
    CBase    to;

    int key = get_next_key( key );
    from.insert( key, new T );
    key = get_next_key( key );
    from.insert( key, new T );
    transfer_test( from, to );
}



template< class BaseContainer, class DerivedContainer, class Derived >
void map_container_assignment_test()
{
    DerivedContainer derived;
    std::string foo( "foo" );
    std::string bar( "foo" );
    derived.insert( foo, new Derived );
    derived.insert( bar, new Derived );

    BaseContainer base_container( derived );
    BOOST_CHECK_EQUAL( derived.size(), base_container.size() );
    base_container.clear();
    base_container = derived;
    BOOST_CHECK_EQUAL( derived.size(), base_container.size() );
    
    BaseContainer base2( base_container );
    BOOST_CHECK_EQUAL( base2.size(), base_container.size() );
    base2 = base_container;
    BOOST_CHECK_EQUAL( base2.size(), base_container.size() );
    base_container = base_container;
}



template< class Cont, class Key, class T >
void test_unordered_interface()
{
    Cont c;
    T* t = new T;
    Key key = get_next_key( key );
    c.insert( key, t );
    typename Cont::local_iterator i = c.begin( 0 );
    typename Cont::const_local_iterator ci = i;
    ci = c.cbegin( 0 );
    i = c.end( 0 );
    ci = c.cend( 0 );
    typename Cont::size_type s = c.bucket_count();
    s = c.max_bucket_count();
    s = c.bucket_size( 0 );
    s = c.bucket( key );
    float f = c.load_factor();
    f       = c.max_load_factor();
    c.max_load_factor(f);
    c.rehash(1000);
} 

#include <boost/ptr_container/ptr_unordered_map.hpp>

using namespace std;

void test_map()
{
    ptr_map_test< ptr_unordered_map<int, Base>, Base, Derived_class >();
    ptr_map_test< ptr_unordered_map<int, Value>, Value, Value >();
    ptr_map_test< ptr_unordered_map<int, nullable<Base> >, Base, Derived_class >();
    ptr_map_test< ptr_unordered_map<int, nullable<Value> >, Value, Value >();
    ptr_map_test< ptr_unordered_map<int, abstract_base>, abstract_base, implementation >();

    ptr_map_test< ptr_unordered_multimap<int,Base>, Base, Derived_class >();
    ptr_map_test< ptr_unordered_multimap<int,Value>, Value, Value >();    
    ptr_map_test< ptr_unordered_multimap<int, nullable<Base> >, Base, Derived_class >();
    ptr_map_test< ptr_unordered_multimap<int, nullable<Value> >, Value, Value >();

    map_container_assignment_test< ptr_unordered_map<std::string,Base>,
                                   ptr_unordered_map<std::string,Derived_class>,
                                   Derived_class>();
    map_container_assignment_test< ptr_unordered_map<std::string, nullable<Base> >,
                                   ptr_unordered_map<std::string,Derived_class>,
                                   Derived_class>();                            
    map_container_assignment_test< ptr_unordered_map<std::string, nullable<Base> >,
                                   ptr_unordered_map<std::string, nullable<Derived_class> >, 
                                   Derived_class>();                               
   map_container_assignment_test< ptr_unordered_multimap<std::string,Base>,
                                   ptr_unordered_multimap<std::string,Derived_class>,
                                   Derived_class>();
    map_container_assignment_test< ptr_unordered_multimap<std::string, nullable<Base> >,
                                   ptr_unordered_multimap<std::string,Derived_class>,
                                   Derived_class>();                            
    map_container_assignment_test< ptr_unordered_multimap<std::string, nullable<Base> >,
                                   ptr_unordered_multimap<std::string, nullable<Derived_class> >, 
                                   Derived_class>();                               

                                      
    test_transfer< ptr_unordered_map<int,Derived_class>, ptr_unordered_map<int,Base>, Derived_class >();
    test_transfer< ptr_unordered_multimap<int,Derived_class>, ptr_unordered_multimap<int,Base>, Derived_class >();
    
    string joe   = "joe";
    string brian = "brian";
    string kenny = "kenny"; 
    
    ptr_unordered_map<string,int> m;
    m.insert( joe, new int( 4 ) );
    m.insert( brian, new int( 6 ) );
    BOOST_CHECK( m[ "foo" ] == 0 );
    m[ "bar" ] += 5;
    BOOST_CHECK( m[ "bar" ] == 5 );
    m[ joe ]   += 56;
    m[ brian ] += 10;

    BOOST_CHECK_THROW( (m.insert(kenny, 0 )), bad_ptr_container_operation );
    BOOST_CHECK_THROW( (m.replace(m.begin(), 0 )), bad_ptr_container_operation ); 
    BOOST_CHECK_THROW( (m.at("not there")), bad_ptr_container_operation );

    for( ptr_unordered_map<string,int>::iterator i = m.begin();
         i != m.end(); ++i )
    {
        if( is_null(i) )
            BOOST_CHECK( false );
        const string& ref  = i->first;
        hide_warning(ref);
        int&          ref2 = *(*i).second;
        ref2++;
    }

    typedef ptr_unordered_map<string,Derived_class> map_type;
    map_type m2;
    m2.insert( joe, new Derived_class );
    //
    // This works fine since 'm2' is not const
    //
    m2.begin()->second->foo();

    //
    // These all return an implementation-defined proxy
    // with two public members: 'first' and 'second'
    //
    map_type::value_type       a_value      = *m2.begin();
    a_value.second->foo();
    map_type::reference        a_reference  = *m2.begin();
    a_reference.second->foo();
    map_type::const_reference  a_creference = *const_begin(m2);
   
    //
    //
    // These will fail as iterators propagate constness
    // 
    //a_creference.second->foo();
    //a_cpointer->second->foo();
    //const_begin(m2)->second->foo();

    test_unordered_interface< ptr_unordered_map<string,Base>, string, Derived_class >();
    test_unordered_interface< ptr_unordered_map<string,Base>, string, Derived_class >();
}

using boost::unit_test::test_suite;

test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite* test = BOOST_TEST_SUITE( "Pointer Container Test Suite" );

    test->add( BOOST_TEST_CASE( &test_map ) );

    return test;
}







