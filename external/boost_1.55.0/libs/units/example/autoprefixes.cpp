// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/** 
\file
    
\brief Example of using autoprefixes.

\details
Example of using engineering (10^3) and binary (2^10) autoprefixes.

Output:
@verbatim
autoprefixes.cpp
using native typeof
Linking...
Embedding manifest...
Autorun "j:\Cpp\Misc\debug\autoprefixes.exe" 
2.345 m
2.345 km
5.49902 MJ
5.49902 megajoule
2.048 kb
2 Kib
2345.6
23456
2345.6
23456
m
meter
0
1
@endverbatim
//[autoprefixes_output

//] [/autoprefixes_output

**/

#include <iostream>

#include <boost/units/io.hpp>
#include <boost/units/pow.hpp>
#include <boost/units/systems/si.hpp>
#include <boost/units/systems/si/io.hpp>
#include <boost/units/quantity.hpp>

struct byte_base_unit : boost::units::base_unit<byte_base_unit, boost::units::dimensionless_type, 3>
{
  static const char* name() { return("byte"); }
  static const char* symbol() { return("b"); }
};

struct thing_base_unit : boost::units::base_unit<thing_base_unit, boost::units::dimensionless_type, 4>
{
  static const char* name() { return("thing"); }
  static const char* symbol() { return(""); }
};

struct euro_base_unit : boost::units::base_unit<euro_base_unit, boost::units::dimensionless_type, 5>
{
  static const char* name() { return("EUR"); }
  static const char* symbol() { return("€"); }
};

int main()
{
  using std::cout;
  using std::endl;

  using namespace boost::units;
  using namespace boost::units::si;

 //[autoprefixes_snippet_1
  using boost::units::binary_prefix;
  using boost::units::engineering_prefix;
  using boost::units::no_prefix;

  quantity<length> l = 2.345 * meters;   // A quantity of length, in units of meters.
  cout << engineering_prefix << l << endl; // Outputs "2.345 m".
  l =  1000.0 * l; // Increase it by 1000, so expect a k prefix.
  // Note that a double 1000.0 is required - an integer will fail to compile.
  cout << engineering_prefix << l << endl; // Output autoprefixed with k to "2.345 km".

  quantity<energy> e = kilograms * pow<2>(l / seconds); // A quantity of energy.
  cout << engineering_prefix << e << endl; // 5.49902 MJ
  cout << name_format << engineering_prefix << e << endl; // 5.49902 megaJoule
  //] [/autoprefixes_snippet_1]

  //[autoprefixes_snippet_2
  // Don't forget that the units name or symbol format specification is persistent.
  cout << symbol_format << endl; // Resets the format to the default symbol format.

  quantity<byte_base_unit::unit_type> b = 2048. * byte_base_unit::unit_type();
  cout << engineering_prefix << b << endl;  // 2.048 kb
  cout << symbol_format << binary_prefix << b << endl; //  "2 Kib" 
  //] [/autoprefixes_snippet_2]

  // Note that scalar dimensionless values are *not* prefixed automatically by the engineering_prefix or binary_prefix iostream manipulators.
  //[autoprefixes_snippet_3
  const double s1 = 2345.6;
  const long x1 = 23456;
  cout << engineering_prefix << s1 << endl; // 2345.6
  cout << engineering_prefix << x1 << endl; // 23456

  cout << binary_prefix << s1 << endl; // 2345.6
  cout << binary_prefix << x1 << endl; // 23456
  //] [/autoprefixes_snippet_3]

  //[autoprefixes_snippet_4
  const length L; // A unit of length (but not a quantity of length).
  cout << L << endl; // Default length unit is meter,
  // but default is symbol format so output is just "m".
  cout << name_format << L << endl; // default length name is "meter".
  //] [/autoprefixes_snippet_4]

  //[autoprefixes_snippet_5
  no_prefix(cout); // Clear any prefix flag.
  cout << no_prefix << endl; // Clear any prefix flag using `no_prefix` manipulator.
  //] [/autoprefixes_snippet_5]

  //[autoprefixes_snippet_6
  cout << boost::units::get_autoprefix(cout) << endl; // 8 is `autoprefix_binary` from `enum autoprefix_mode`.
  cout << boost::units::get_format(cout) << endl; // 1 is `name_fmt` from `enum format_mode`.
  //] [/autoprefixes_snippet_6]


  quantity<thing_base_unit::unit_type> t = 2048. * thing_base_unit::unit_type();
  cout << name_format << engineering_prefix << t << endl;  // 2.048 kilothing
  cout << symbol_format << engineering_prefix << t << endl;  // 2.048 k
 
  cout  << binary_prefix << t << endl; //  "2 Ki" 

  quantity<euro_base_unit::unit_type> ce = 2048. * euro_base_unit::unit_type();
  cout << name_format << engineering_prefix << ce << endl;  // 2.048 kiloEUR
  cout << symbol_format << engineering_prefix << ce << endl;  // 2.048 k€


    return 0;
} // int main()

