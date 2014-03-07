
// Copyright 2005 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software 
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//
// dynamic_properties_test.cpp - test cases for the dynamic property maps.
//

//  Author: Ronald Garcia
#include <boost/config.hpp>

// For Borland, act like BOOST_NO_EXPLICIT_FUNCTION_TEMPLATE_ARGUMENTS is defined
#if defined (__BORLANDC__) && (__BORLANDC__ <= 0x570) && !defined(BOOST_NO_EXPLICIT_FUNCTION_TEMPLATE_ARGUMENTS)
#  define BOOST_NO_EXPLICIT_FUNCTION_TEMPLATE_ARGUMENTS
#endif

#include <boost/test/minimal.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/property_map/dynamic_property_map.hpp>
#include <boost/property_map/property_map.hpp>
#include <map>
#include <iostream>
#include <string>
#include <memory>

// generate a dynamic_property_map that maps strings to strings
// WARNING: This code leaks memory.  For testing purposes only!
// WARNING: This code uses library internals. For testing purposes only!
boost::shared_ptr<boost::dynamic_property_map>
string2string_gen(const std::string& name,
                  const boost::any&,
                  const boost::any&) {
  typedef std::map<std::string,std::string> map_t;
  typedef
    boost::associative_property_map< std::map<std::string, std::string> >
    property_t;


  map_t* mymap = new map_t(); // hint: leaky memory here!

  property_t property_map(*mymap);

  boost::shared_ptr<boost::dynamic_property_map> pm(
    new
    boost::detail::dynamic_property_map_adaptor<property_t>(property_map));

  return pm;
}


int test_main(int,char**) {

  // build property maps using associative_property_map

  std::map<std::string, int> string2int;
  std::map<double,std::string> double2string;
  boost::associative_property_map< std::map<std::string, int> >
    int_map(string2int);
  boost::associative_property_map< std::map<double, std::string> >
    dbl_map(double2string);


  // add key-value information
  string2int["one"] = 1;
  string2int["five"] = 5;
  
  double2string[5.3] = "five point three";
  double2string[3.14] = "pi";
 
 
  // build and populate dynamic interface
  boost::dynamic_properties properties;
  properties.property("int",int_map);
  properties.property("double",dbl_map);
  
  using boost::get;
  using boost::put;
  using boost::type;
  // Get tests
  {
    BOOST_CHECK(get("int",properties,std::string("one")) == "1");
#ifndef BOOST_NO_EXPLICIT_FUNCTION_TEMPLATE_ARGUMENTS
    BOOST_CHECK(boost::get<int>("int",properties,std::string("one")) == 1);
#endif
    BOOST_CHECK(get("int",properties,std::string("one"), type<int>()) == 1);
    BOOST_CHECK(get("double",properties,5.3) == "five point three");
  }

  // Put tests
  {
    put("int",properties,std::string("five"),6);
    BOOST_CHECK(get("int",properties,std::string("five")) == "6");
    put("int",properties,std::string("five"),std::string("5"));
#ifndef BOOST_NO_EXPLICIT_FUNCTION_TEMPLATE_ARGUMENTS
    BOOST_CHECK(get<int>("int",properties,std::string("five")) == 5);
#endif
    BOOST_CHECK(get("int",properties,std::string("five"),type<int>()) == 5);
    put("double",properties,3.14,std::string("3.14159"));
    BOOST_CHECK(get("double",properties,3.14) == "3.14159");
    put("double",properties,3.14,std::string("pi"));
#ifndef BOOST_NO_EXPLICIT_FUNCTION_TEMPLATE_ARGUMENTS
    BOOST_CHECK(get<std::string>("double",properties,3.14) == "pi");
#endif
    BOOST_CHECK(get("double",properties,3.14,type<std::string>()) == "pi");
  }

  // Nonexistent property
  {
    try {
      get("nope",properties,3.14);
      BOOST_ERROR("No exception thrown.");
    } catch (boost::dynamic_get_failure&) { }

    try {
      put("nada",properties,3.14,std::string("3.14159"));
      BOOST_ERROR("No exception thrown.");
    } catch (boost::property_not_found&) { }
  }

  // Nonexistent property gets generated
  {
    boost::dynamic_properties props(&string2string_gen);
    put("nada",props,std::string("3.14"),std::string("pi"));
    BOOST_CHECK(get("nada",props,std::string("3.14"))  == "pi");
  }

  // Use the ignore_other_properties generator
  {
    boost::dynamic_properties props(&boost::ignore_other_properties);
    bool value = put("nada",props,std::string("3.14"),std::string("pi"));
    BOOST_CHECK(value == false);
  }
 
  return boost::exit_success;
}
