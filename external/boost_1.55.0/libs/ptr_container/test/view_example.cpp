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

//
// This example is intended to show you how to
// use the 'view_clone_manager'. The idea
// is that we have a container of non-polymorphic
// objects and want to keep then sorted by different
// criteria at the same time. 
//

//
// We'll go for 'ptr_vector' here. Using a node-based 
// container would be a waste of space here.
// All container headers will also include
// the Clone Managers.
// 
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/ptr_container/indirect_fun.hpp>

#include <functional> // For 'binary_fnuction'
#include <cstdlib>    // For 'rand()'
#include <algorithm>  // For 'std::sort()'
#include <iostream>   // For 'std::cout'

using namespace std;

//
// This is our simple example data-structure. It can
// be ordered in three ways.
//
struct photon
{
    photon() : color( rand() ), 
               direction( rand() ),
               power( rand() )
    { }
    
    int color;
    int direction;
    int power;
};

//
// Our big container is a standard vector
//
typedef std::vector<photon>                                 vector_type;

//
// Now we define our view type by adding a second template argument.
// The 'view_clone_manager' will implements Cloning by taking address
// of objects.
//
// Notice the first template argument is 'photon' and not
// 'const photon' to allow the view container write access.
//
typedef boost::ptr_vector<photon,boost::view_clone_allocator> view_type;

//
// Our first sort criterium
//
struct sort_by_color : std::binary_function<photon,photon,bool>
{
    bool operator()( const photon& l, const photon& r ) const
    {
        return l.color < r.color;
    }
};

//
// Our second sort criterium
//
struct sort_by_direction : std::binary_function<photon,photon,bool>
{
    bool operator()( const photon& l, const photon& r ) const
    {
        return l.direction < r.direction;
    }
};


//
// Our third sort criterium
//
struct sort_by_power : std::binary_function<photon,photon,bool>
{
    bool operator()( const photon& l, const photon& r ) const
    {
        return l.power < r.power;
    }
};

//
// This function inserts "Clones" into the
// the view. 
//
// We need to pass the first argument
// as a non-const reference to be able to store
// 'T*' instead of 'const T*' objects. Alternatively,
// we might change the declaration of the 'view_type'
// to 
//     typedef boost::ptr_vector<const photon,boost::view_clone_manager> 
//               view_type;     ^^^^^^
//
void insert( vector_type& from, view_type& to )
{
        to.insert( to.end(), 
                   from.begin(),
                   from.end() );
}

int main()
{
    enum { sz = 10, count = 500 };

    //
    // First we create the main container and two views
    //
    std::vector<vector_type>  photons;
    view_type                 color_view;
    view_type                 direction_view;

    //
    // Then we fill the main container with some random data
    //
    for( int i = 0; i != sz; ++i )
    {
        photons.push_back( vector_type() ); 

        for( int j = 0; j != count; ++j )
            photons[i].push_back( photon() );
    }

    //
    // Then we create the two views.
    //
    for( int i = 0; i != sz; ++i )
    {
        insert( photons[i], color_view );
        insert( photons[i], direction_view );
    }

    //
    // First we sort the original photons, using one of
    // the view classes. This may sound trivial, but consider that
    // the objects are scatered all around 'sz' different vectors;
    // the view makes them act as one big vector.
    //
    std::sort( color_view.begin(), color_view.end(), sort_by_power() );
    
    //
    // And now we can sort the views themselves. Notice how
    // we switch to different iterators and different predicates:
    //
    color_view.sort( sort_by_color() );

    direction_view.sort( sort_by_direction() );

    return 0;
}
