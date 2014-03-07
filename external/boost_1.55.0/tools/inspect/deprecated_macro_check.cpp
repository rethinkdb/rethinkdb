//  deprecated macro check implementation  ---------------------------------------------//
//	Protect against ourself: boostinspect:ndprecated_macros

//  Copyright Eric Niebler 2010.
//  Based on the assert_macro_check checker by Marshall Clow
//
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include "deprecated_macro_check.hpp"
#include <functional>
#include "boost/regex.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/filesystem/operations.hpp"

namespace fs = boost::filesystem;

namespace
{
  const char * boost150macros [] = {
	"BOOST_NO_0X_HDR_ARRAY",
	"BOOST_NO_0X_HDR_CHRONO",
	"BOOST_NO_0X_HDR_CODECVT",
	"BOOST_NO_0X_HDR_CONDITION_VARIABLE",
	"BOOST_NO_0X_HDR_FORWARD_LIST",
	"BOOST_NO_0X_HDR_FUTURE",
	"BOOST_NO_0X_HDR_INITIALIZER_LIST",
	"BOOST_NO_INITIALIZER_LISTS",
	"BOOST_NO_0X_HDR_MUTEX",
	"BOOST_NO_0X_HDR_RANDOM",
	"BOOST_NO_0X_HDR_RATIO",
	"BOOST_NO_0X_HDR_REGEX",
	"BOOST_NO_0X_HDR_SYSTEM_ERROR",
	"BOOST_NO_0X_HDR_THREAD",
	"BOOST_NO_0X_HDR_TUPLE",
	"BOOST_NO_0X_HDR_TYPE_TRAITS",
	"BOOST_NO_0X_HDR_TYPEINDEX",
	"BOOST_NO_0X_HDR_UNORDERED_SET",
	"BOOST_NO_0X_HDR_UNORDERED_MAP",
	"BOOST_NO_STD_UNORDERED",
	NULL
	};
	
  const char * boost151macros [] = {
	"BOOST_NO_AUTO_DECLARATIONS",
	"BOOST_NO_AUTO_MULTIDECLARATIONS",
	"BOOST_NO_CHAR16_T",
	"BOOST_NO_CHAR32_T",
	"BOOST_NO_TEMPLATE_ALIASES",
	"BOOST_NO_CONSTEXPR",
	"BOOST_NO_DECLTYPE",
	"BOOST_NO_DECLTYPE_N3276",
	"BOOST_NO_DEFAULTED_FUNCTIONS",
	"BOOST_NO_DELETED_FUNCTIONS",
	"BOOST_NO_EXPLICIT_CONVERSION_OPERATORS",
	"BOOST_NO_EXTERN_TEMPLATE",
	"BOOST_NO_FUNCTION_TEMPLATE_DEFAULT_ARGS",
	"BOOST_NO_LAMBDAS",
	"BOOST_NO_LOCAL_CLASS_TEMPLATE_PARAMETERS",
	"BOOST_NO_NOEXCEPT",
	"BOOST_NO_NULLPTR",
	"BOOST_NO_RAW_LITERALS",
	"BOOST_NO_RVALUE_REFERENCES",
	"BOOST_NO_SCOPED_ENUMS",
	"BOOST_NO_STATIC_ASSERT",
	"BOOST_NO_STD_UNORDERED",
	"BOOST_NO_UNICODE_LITERALS",
	"BOOST_NO_UNIFIED_INITIALIZATION_SYNTAX",
	"BOOST_NO_VARIADIC_TEMPLATES",
	"BOOST_NO_VARIADIC_MACROS",
	"BOOST_NO_NUMERIC_LIMITS_LOWEST",
    NULL
    };

  const char * boost153macros [] = {
	"BOOST_HAS_STATIC_ASSERT",
	"BOOST_HAS_RVALUE_REFS",
	"BOOST_HAS_VARIADIC_TMPL",
	"BOOST_HAS_CHAR_16_T",
	"BOOST_HAS_CHAR_32_T",
    NULL
    };
} // unnamed namespace


namespace boost
{
  namespace inspect
  {
   deprecated_macro_check::deprecated_macro_check()
     : m_files_with_errors(0)
     , m_from_boost_root(
         fs::exists(fs::initial_path() / "boost") &&
         fs::exists(fs::initial_path() / "libs"))
   {
     register_signature( ".c" );
     register_signature( ".cpp" );
     register_signature( ".cxx" );
     register_signature( ".h" );
     register_signature( ".hpp" );
     register_signature( ".hxx" );
     register_signature( ".ipp" );
   }

   void deprecated_macro_check::inspect(
      const string & library_name,
      const path & full_path,   // example: c:/foo/boost/filesystem/path.hpp
      const string & contents )     // contents of file to be inspected
    {
      if (contents.find( "boostinspect:" "ndprecated_macros" ) != string::npos)
        return;

      const char **ptr;
      long errors = 0;
      for ( ptr = boost150macros; *ptr != NULL; ++ptr )
      {
		if ( contents.find( *ptr ) != string::npos ) {
          ++errors;
          error( library_name, full_path, string ( "Boost macro deprecated in 1.50: " ) + *ptr );
          }
      }

      for ( ptr = boost151macros; *ptr != NULL; ++ptr )
      {
		if ( contents.find( *ptr ) != string::npos ) {
          ++errors;
          error( library_name, full_path, string ( "Boost macro deprecated in 1.51: " ) + *ptr );
          }
      }

      for ( ptr = boost153macros; *ptr != NULL; ++ptr )
      {
		if ( contents.find( *ptr ) != string::npos ) {
          ++errors;
          error( library_name, full_path, string ( "Boost macro deprecated in 1.53: " ) + *ptr );
          }
      }

      if(errors > 0)
        ++m_files_with_errors;
    }
  } // namespace inspect
} // namespace boost


