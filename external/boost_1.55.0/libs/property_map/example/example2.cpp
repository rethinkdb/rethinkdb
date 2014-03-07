//  (C) Copyright Ronald Garcia, Jeremy Siek 2002. 
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <map>
#include <string>
#include <boost/property_map/property_map.hpp>


template <typename ConstAddressMap>
void display(ConstAddressMap address)
{
  typedef typename boost::property_traits<ConstAddressMap>::value_type
    value_type;
  typedef typename boost::property_traits<ConstAddressMap>::key_type key_type;

  key_type fred = "Fred";
  key_type joe = "Joe";

  value_type freds_address = get(address, fred);
  value_type joes_address = get(address, joe);
  
  std::cout << fred << ": " << freds_address << "\n"
            << joe  << ": " << joes_address  << "\n";
}

int
main()
{
  std::map<std::string, std::string> name2address;
  boost::const_associative_property_map< std::map<std::string, std::string> >
    address_map(name2address);

  name2address.insert(make_pair(std::string("Fred"), 
                                std::string("710 West 13th Street")));
  name2address.insert(make_pair(std::string("Joe"), 
                                std::string("710 West 13th Street")));

  display(address_map);
  
  return EXIT_SUCCESS;
}


