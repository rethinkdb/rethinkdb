//  (C) Copyright Jeremy Siek 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <map>
#include <string>
#include <boost/property_map/property_map.hpp>


template <typename AddressMap>
void foo(AddressMap address)
{
  typedef typename boost::property_traits<AddressMap>::value_type value_type;
  typedef typename boost::property_traits<AddressMap>::key_type key_type;

  value_type old_address, new_address;
  key_type fred = "Fred";
  old_address = get(address, fred);
  new_address = "384 Fitzpatrick Street";
  put(address, fred, new_address);

  key_type joe = "Joe";
  value_type& joes_address = address[joe];
  joes_address = "325 Cushing Avenue";
}

int
main()
{
  std::map<std::string, std::string> name2address;
  boost::associative_property_map< std::map<std::string, std::string> >
    address_map(name2address);

  name2address.insert(make_pair(std::string("Fred"), 
                                std::string("710 West 13th Street")));
  name2address.insert(make_pair(std::string("Joe"), 
                                std::string("710 West 13th Street")));

  foo(address_map);
  
  for (std::map<std::string, std::string>::iterator i = name2address.begin();
       i != name2address.end(); ++i)
    std::cout << i->first << ": " << i->second << "\n";

  return EXIT_SUCCESS;
}
