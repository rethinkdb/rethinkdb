// Copyright (C) 2003, Fernando Luis Cacciola Carballal.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//


//
// NOTE: This file is intended to be used ONLY by the test files
//       from the Numeric Conversions Library
//

//
// The following 'to_string' helpers are provided to give readable names
// to the various enums used by the library.
// NOTE: specializations of boost::lexical_cast<> were not used since some compilers had
// trouble dealing with such specializations for different enumerations.
//

std::string to_string ( boost::numeric::int_float_mixture_enum arg )
{
  switch ( arg )
  {
    case boost::numeric::integral_to_integral : return "integral_to_integral" ;
    case boost::numeric::integral_to_float    : return "integral_to_float" ;
    case boost::numeric::float_to_integral    : return "float_to_integral" ;
    case boost::numeric::float_to_float       : return "float_to_float" ;
  }
  return "(Unknown result!)" ;
}


std::string to_string ( boost::numeric::sign_mixture_enum arg )
{
  switch ( arg )
  {
    case boost::numeric::unsigned_to_unsigned : return "unsigned_to_unsigned" ;
    case boost::numeric::signed_to_signed     : return "signed_to_signed" ;
    case boost::numeric::signed_to_unsigned   : return "signed_to_unsigned" ;
    case boost::numeric::unsigned_to_signed   : return "unsigned_to_signed" ;
  }
  return "(Unknown result!)" ;
}

std::string to_string ( boost::numeric::udt_builtin_mixture_enum arg )
{
  switch ( arg )
  {
    case boost::numeric::builtin_to_builtin : return "builtin_to_builtin" ;
    case boost::numeric::builtin_to_udt     : return "builtin_to_udt" ;
    case boost::numeric::udt_to_builtin     : return "udt_to_builtin" ;
    case boost::numeric::udt_to_udt         : return "udt_to_udt" ;
  }
  return "(Unknown result!)" ;
}

//
///////////////////////////////////////////////////////////////////////////////////////////////

