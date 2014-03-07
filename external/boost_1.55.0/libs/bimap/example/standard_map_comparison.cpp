// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  VC++ 8.0 warns on usage of certain Standard Library and API functions that
//  can be cause buffer overruns or other possible security issues if misused.
//  See http://msdn.microsoft.com/msdnmag/issues/05/05/SafeCandC/default.aspx
//  But the wording of the warning is misleading and unsettling, there are no
//  portable alternative functions, and VC++ 8.0's own libraries use the
//  functions in question. So turn off the warnings.
#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_DEPRECATE

// Boost.Bimap Example
//-----------------------------------------------------------------------------

#include <boost/config.hpp>

#include <string>
#include <iostream>

#include <map>
#include <boost/bimap/bimap.hpp>

using namespace boost::bimaps;

//[ code_standard_map_comparison

template< class Map, class CompatibleKey, class CompatibleData >
void use_it( Map & m,
             const CompatibleKey  & key,
             const CompatibleData & data )
{
    typedef typename Map::value_type value_type;
    typedef typename Map::const_iterator const_iterator;

    m.insert( value_type(key,data) );
    const_iterator iter = m.find(key);
    if( iter != m.end() )
    {
        assert( iter->first  == key  );
        assert( iter->second == data );

        std::cout << iter->first << " --> " << iter->second;
    }
    m.erase(key);
}

int main()
{
    typedef bimap< set_of<std::string>, set_of<int> > bimap_type;
    bimap_type bm;

    // Standard map
    {
        typedef std::map< std::string, int > map_type;
        map_type m;

        use_it( m, "one", 1 );
    }

    // Left map view
    {
        typedef bimap_type::left_map map_type;
        map_type & m = bm.left;

        use_it( m, "one", 1 );
    }

    // Reverse standard map
    {
        typedef std::map< int, std::string > reverse_map_type;
        reverse_map_type rm;

        use_it( rm, 1, "one" );
    }

    // Right map view
    {
        typedef bimap_type::right_map reverse_map_type;
        reverse_map_type & rm = bm.right;

        use_it( rm, 1, "one" );
    }

    return 0;
}
//]

