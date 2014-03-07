
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef TREE_H
#define TREE_H

#include <cstddef>
#include <string>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/intrusive_ptr.hpp>

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4355)
# endif

struct branch;
struct leaf;

struct visitor
{
    virtual ~visitor() {};

    virtual void visit( branch & b) = 0;
    virtual void visit( leaf & l) = 0;
};

struct node
{
    typedef boost::intrusive_ptr< node >    ptr_t;

    std::size_t     use_count;

    node() :
        use_count( 0)
    {}

    virtual ~node() {}

    virtual void accept( visitor & v) = 0;

    friend inline void intrusive_ptr_add_ref( node * p)
    { ++p->use_count; }

    friend inline void intrusive_ptr_release( node * p)
    { if ( 0 == --p->use_count) delete p; }
};

struct branch : public node
{
    node::ptr_t     left;
    node::ptr_t     right;

    static ptr_t create( node::ptr_t left_, node::ptr_t right_)
    { return ptr_t( new branch( left_, right_) ); }

    branch( node::ptr_t left_, node::ptr_t right_) :
        left( left_), right( right_)
    {}

    void accept( visitor & v)
    { v.visit( * this); }
};

struct leaf : public node
{
    std::string     value;

    static ptr_t create( std::string const& value_)
    { return ptr_t( new leaf( value_) ); }

    leaf( std::string const& value_) :
        value( value_)
    {}

    void accept( visitor & v)
    { v.visit( * this); }
};

inline
bool operator==( leaf const& l, leaf  const& r)
{ return l.value == r.value; }

inline
bool operator!=( leaf const& l, leaf  const& r)
{ return l.value != r.value; }

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#endif // TREE_H
