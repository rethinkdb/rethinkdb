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
#include <boost/lexical_cast.hpp>
#include <algorithm>
#include <iostream>
#include <cstddef>
#include <string>

using namespace std;
using namespace boost;

class node;

class tree
{ 
    typedef ptr_vector<node> nodes_t;
    nodes_t nodes; 

protected:
        void swap( tree& r )               
            { nodes.swap( r.nodes ); } 

public:
    typedef nodes_t::iterator       iterator;
    typedef nodes_t::const_iterator const_iterator;
         
public:
    void            add_child( node* n );
    void            remove( iterator n );
    void            write_tree( ostream& os ) const;
    size_t          size() const;
    node&           child( size_t idx );
    const node&     child( size_t idx ) const;
    iterator        child_begin();
    const_iterator  child_begin() const;
    iterator        child_end();
    const_iterator  child_end() const;
    iterator        find( const string& match );
};



class node : noncopyable
{
    virtual size_t  do_node_size() const = 0;
    virtual string  do_description() const = 0;
    virtual void    do_write_value( ostream& os ) const = 0;
    
public:
    virtual  ~node()                           { }
    size_t   node_size() const                 { return do_node_size(); }
    string   description() const               { return do_description(); }
    void     write_value( ostream& os ) const  { do_write_value( os ); }
};



class inner_node : public node, public tree
{
    string name;

    virtual size_t do_node_size() const
    {
        return tree::size();
    }
    
    virtual string do_description() const
    {
        return lexical_cast<string>( name );
    }
    
    virtual void do_write_value( ostream& os ) const
    {
        os << " inner node: " << name;
    }

    void swap( inner_node& r )
    {
        name.swap( r.name );
        tree::swap( r );
    }
    
public:
    inner_node() : name("inner node") 
    { }

    inner_node( const string& r ) : name(r)
    { }

    inner_node* release()
    {
        inner_node* ptr( new inner_node );
        ptr->swap( *this );
        return ptr;
    }
};
 


template< class T >
class leaf : public node
{
    T data;
    
    virtual size_t do_node_size() const
    {
        return 1;
    }
    
    virtual string do_description() const
    {
        return lexical_cast<string>( data );
    }

    virtual void do_write_value( ostream& os ) const
    {
        os << " leaf: " << data;
    }

public:
    leaf() : data( T() )
    { }

    leaf( const T& r ) : data(r)
    { }

    void set_data( const T& r )           
    { data = r; }
};

/////////////////////////////////////////////////////////////////////////
// tree implementation
/////////////////////////////////////////////////////////////////////////

inline void tree::add_child( node* n )
{
    nodes.push_back( n );
}

inline void tree::remove( iterator n )
{
    BOOST_ASSERT( n != nodes.end() );
    nodes.erase( n );
}

void tree::write_tree( ostream& os ) const
{
    for( const_iterator i = nodes.begin(),
                        e = nodes.end();
         i != e; ++i )
    {
        i->write_value( os );
        if( const inner_node* p = dynamic_cast<const inner_node*>( &*i ) )
            p->write_tree( os );
    }
}

size_t tree::size() const
{
    size_t res = 1;

    for( const_iterator i = nodes.begin(),
                        e = nodes.end();
         i != e; ++i )
    {
        res += i->node_size();
    }

    return res;
}

inline node& tree::child( size_t idx )
{
    return nodes[idx];
}

inline const node& tree::child( size_t idx ) const
{
    return nodes[idx];
}

inline tree::iterator tree::child_begin()
{
    return nodes.begin();
}

inline tree::const_iterator tree::child_begin() const
{
    return nodes.begin();
}

inline tree::iterator tree::child_end()
{
    return nodes.end();
}

inline tree::const_iterator tree::child_end() const
{
    return nodes.end();
}

tree::iterator tree::find( const string& match )
{
    for( iterator i = nodes.begin(),
                  e = nodes.end();
         i != e; ++i )
    {
        if( i->description() == match )
            return i;

        if( inner_node* p = dynamic_cast<inner_node*>( &*i ) )
        {
            iterator j = p->find( match );
            if( j != p->child_end() )
                return j;
        }
        
    }

    return child_end();
}

/////////////////////////////////////////////////////////////////////////
// test case
/////////////////////////////////////////////////////////////////////////

void test_tree()
{
    tree root;
    BOOST_CHECK_EQUAL( root.size(), 1u );
    inner_node node1( "node 1" );
    node1.add_child( new leaf<string>( "leaf 1" ) );
    node1.add_child( new leaf<int>( 42 ) );
    inner_node node2( "node 2" );
    node2.add_child( new leaf<float>( 42.0f ) );
    node2.add_child( new leaf<string>( "leaf 4" ) );
    
    root.add_child( node1.release() );
    BOOST_CHECK_EQUAL( root.size(), 4u );
    root.add_child( node2.release() );
    root.add_child( new inner_node( "node 3" ) );
    BOOST_CHECK_EQUAL( root.size(), 8u );
    root.add_child( new leaf<string>( "leaf 5" ) );
    BOOST_CHECK_EQUAL( root.size(), 9u );

    root.write_tree( cout );
    tree::iterator a_leaf = root.find( "42" );
    BOOST_CHECK_EQUAL( a_leaf->description(), "42" );
    leaf<int>& the_leaf = dynamic_cast< leaf<int>& >( *a_leaf );
    the_leaf.set_data( 2*42 );
    BOOST_CHECK_EQUAL( a_leaf->description(), "84" );

}

#include <boost/test/unit_test.hpp>
using boost::unit_test::test_suite;

test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite* test = BOOST_TEST_SUITE( "Pointer Container Test Suite" );

    test->add( BOOST_TEST_CASE( &test_tree ) );

    return test;
}


