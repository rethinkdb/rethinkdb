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

#include <iostream>
#include <string>

#include <boost/bimap/bimap.hpp>
#include <boost/bimap/multiset_of.hpp>
#include <boost/bimap/property_map/set_support.hpp>

using namespace boost::bimaps;

//[ code_bimap_and_boost_property_map

template <typename AddressMap>
void foo(AddressMap & address_map)
{
    typedef typename boost::property_traits<AddressMap>::value_type value_type;
    typedef typename boost::property_traits<AddressMap>::key_type key_type;

    value_type address;
    key_type fred = "Fred";
    std::cout << get(address_map, fred);
}

int main()
{
    typedef bimap<std::string, multiset_of<std::string> > Name2Address;
    typedef Name2Address::value_type location;

    Name2Address name2address;
    name2address.insert( location("Fred", "710 West 13th Street") );
    name2address.insert( location( "Joe", "710 West 13th Street") );

    foo( name2address.left );

    return 0;
}
//]

