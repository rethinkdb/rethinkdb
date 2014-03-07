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

#include <boost/ptr_container/ptr_sequence_adapter.hpp>
#include <vector>
#include <boost/ptr_container/ptr_map_adapter.hpp>
#include <map>

template< class T >
struct my_ptr_vector : 
    public boost::ptr_sequence_adapter< std::vector<T*> >
{

};

    
template< class Key, class T, class Pred = std::less<Key>,
          class Allocator = std::allocator< std::pair<const Key, T> > >
struct my_map : public std::map<Key,T,Pred,Allocator>
{
    explicit my_map( const Pred&      pred  = Pred(), 
                     const Allocator& alloc = Allocator() ) 
    { }
};

#include <string>
struct Foo {};

typedef boost::ptr_map_adapter< my_map<std::string,Foo*> > foo_map;

template< class Key, class T, class Pred = std::less<Key> >
struct my_ptr_map : public boost::ptr_map_adapter< std::map<Key,T*,Pred> >
{

};

typedef my_ptr_map<std::string,Foo> foo_map2;
    

int main()
{
    
    my_ptr_vector<Foo> vec;
    vec.push_back( new Foo );
    foo_map  m1;
    foo_map2 m2;
    std::string s("");
    m1.insert( s, new Foo );
    m2.insert( s, new Foo );

    
}

