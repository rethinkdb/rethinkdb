//  error_code_test.cpp  -----------------------------------------------------//

//  Copyright Beman Dawes 2006

//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See library home page at http://www.boost.org/libs/system

//----------------------------------------------------------------------------//

//  test without deprecated features
#define BOOST_SYSTEM_NO_DEPRECATED

#include <boost/config/warning_disable.hpp>

#include <boost/detail/lightweight_test.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/cygwin_error.hpp>
#include <boost/system/linux_error.hpp>
#include <boost/system/windows_error.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <functional>
#include <boost/cerrno.hpp>

//  Although using directives are not the best programming practice, testing
//  with a boost::system using directive increases use scenario coverage.
using namespace boost::system;

# if defined( BOOST_WINDOWS_API )
#   include "winerror.h"
#   define BOOST_ACCESS_ERROR_MACRO ERROR_ACCESS_DENIED
# elif defined( BOOST_POSIX_API )
#   define BOOST_ACCESS_ERROR_MACRO EACCES
# else
#   error "Only supported for POSIX and Windows"
# endif

namespace
{
  void check_ostream( error_code ec, const char * expected )
  {
    std::stringstream ss;
    std::string s;

    ss << ec;
    ss >> s;
    BOOST_TEST( s == expected );
  }
}

//  main  ------------------------------------------------------------------------------//

// TODO: add hash_value tests

int main( int, char ** )
{

  std::cout << "Conversion use cases...\n";
  error_condition x1( errc::file_exists );
  //error_code x2( errc::file_exists ); // should fail to compile
  make_error_code(errc::file_exists);
  make_error_condition(errc::file_exists);

  std::cout << "General tests...\n";
  // unit tests:

  BOOST_TEST( generic_category() == generic_category() );
  BOOST_TEST( system_category() == system_category() );
  BOOST_TEST( generic_category() != system_category() );
  BOOST_TEST( system_category() != generic_category() );

  if ( std::less<const error_category*>()( &generic_category(), &system_category() ) )
  {
    BOOST_TEST( generic_category() < system_category() );
    BOOST_TEST( !(system_category() < generic_category()) );
  }
  else
  {
    BOOST_TEST( system_category() < generic_category() );
    BOOST_TEST( !(generic_category() < system_category()) );
  }


  error_code ec;
  error_condition econd;
  BOOST_TEST( !ec );
  BOOST_TEST( ec.value() == 0 );
  econd = ec.default_error_condition();
  BOOST_TEST( econd.value() == 0 );
  BOOST_TEST( econd.category() == generic_category() );
  BOOST_TEST( ec == errc::success );
  BOOST_TEST( ec.category() == system_category() );
  BOOST_TEST( std::strcmp( ec.category().name(), "system") == 0 );
  BOOST_TEST( !(ec < error_code( 0, system_category() )) );
  BOOST_TEST( !(error_code( 0, system_category() ) < ec) );
  BOOST_TEST( ec < error_code( 1, system_category() ) );
  BOOST_TEST( !(error_code( 1, system_category() ) < ec) );

  error_code ec_0_system( 0, system_category() );
  BOOST_TEST( !ec_0_system );
  BOOST_TEST( ec_0_system.value() == 0 );
  econd = ec_0_system.default_error_condition();
  BOOST_TEST( econd.value() == 0 );
  BOOST_TEST( econd.category() == generic_category() );
  BOOST_TEST( ec_0_system == errc::success );
  BOOST_TEST( ec_0_system.category() == system_category() );
  BOOST_TEST( std::strcmp( ec_0_system.category().name(), "system") == 0 );
  check_ostream( ec_0_system, "system:0" );

  BOOST_TEST( ec_0_system == ec );

  error_code ec_1_system( 1, system_category() );
  BOOST_TEST( ec_1_system );
  BOOST_TEST( ec_1_system.value() == 1 );
  BOOST_TEST( ec_1_system.value() != 0 );
  BOOST_TEST( ec != ec_1_system );
  BOOST_TEST( ec_0_system != ec_1_system );
  check_ostream( ec_1_system, "system:1" );

  ec = error_code( BOOST_ACCESS_ERROR_MACRO, system_category() );
  BOOST_TEST( ec );
  BOOST_TEST( ec.value() == BOOST_ACCESS_ERROR_MACRO );
  econd = ec.default_error_condition();
  BOOST_TEST( econd.value() == static_cast<int>(errc::permission_denied) );
  BOOST_TEST( econd.category() == generic_category() );
  BOOST_TEST( econd == error_condition( errc::permission_denied, generic_category() ) );
  BOOST_TEST( econd == errc::permission_denied );
  BOOST_TEST( errc::permission_denied == econd );
  BOOST_TEST( ec == errc::permission_denied );
  BOOST_TEST( ec.category() == system_category() );
  BOOST_TEST( std::strcmp( ec.category().name(), "system") == 0 );

  // test the explicit make_error_code conversion for errc
  ec = make_error_code( errc::bad_message );
  BOOST_TEST( ec );
  BOOST_TEST( ec == errc::bad_message );
  BOOST_TEST( errc::bad_message == ec );
  BOOST_TEST( ec != errc::permission_denied );
  BOOST_TEST( errc::permission_denied != ec );
  BOOST_TEST( ec.category() == generic_category() );

  //// test the deprecated predefined error_category synonyms
  //BOOST_TEST( &system_category() == &native_ecat );
  //BOOST_TEST( &generic_category() == &errno_ecat );
  //BOOST_TEST( system_category() == native_ecat );
  //BOOST_TEST( generic_category() == errno_ecat );

  // test error_code and error_condition message();
  // see Boost.Filesystem operations_test for code specific message() tests
  ec = error_code( -1, system_category() );
  std::cout << "error_code message for -1 is \"" << ec.message() << "\"\n";
  std::cout << "error_code message for 0 is \"" << ec_0_system.message() << "\"\n";
#if defined(BOOST_WINDOWS_API)
  // Borland appends newline, so just check text
  BOOST_TEST( ec.message().substr(0,13) == "Unknown error" );
  BOOST_TEST( ec_0_system.message().substr(0,36) == "The operation completed successfully" );
#elif  defined(linux) || defined(__linux) || defined(__linux__)
  // Linux appends value to message as unsigned, so it varies with # of bits
  BOOST_TEST( ec.message().substr(0,13) == "Unknown error" );
#elif defined(__hpux)
  BOOST_TEST( ec.message() == "" );
#elif defined(__osf__)
  BOOST_TEST( ec.message() == "Error -1 occurred." );
#elif defined(__vms)
  BOOST_TEST( ec.message() == "error -1" );
#endif

  ec = error_code( BOOST_ACCESS_ERROR_MACRO, system_category() );
  BOOST_TEST( ec.message() != "" );
  BOOST_TEST( ec.message().substr( 0, 13) != "Unknown error" );

  econd = error_condition( -1, generic_category() );
  error_condition econd_ok;
  std::cout << "error_condition message for -1 is \"" << econd.message() << "\"\n";
  std::cout << "error_condition message for 0 is \"" << econd_ok.message() << "\"\n";
#if defined(BOOST_WINDOWS_API)
  // Borland appends newline, so just check text
  BOOST_TEST( econd.message().substr(0,13) == "Unknown error" );
  BOOST_TEST( econd_ok.message().substr(0,8) == "No error" );
#elif  defined(linux) || defined(__linux) || defined(__linux__)
  // Linux appends value to message as unsigned, so it varies with # of bits
  BOOST_TEST( econd.message().substr(0,13) == "Unknown error" );
#elif defined(__hpux)
  BOOST_TEST( econd.message() == "" );
#elif defined(__osf__)
  BOOST_TEST( econd.message() == "Error -1 occurred." );
#elif defined(__vms)
  BOOST_TEST( econd.message() == "error -1" );
#endif

  econd = error_condition( BOOST_ACCESS_ERROR_MACRO, generic_category() );
  BOOST_TEST( econd.message() != "" );
  BOOST_TEST( econd.message().substr( 0, 13) != "Unknown error" );

#ifdef BOOST_WINDOWS_API
  std::cout << "Windows tests...\n";
  // these tests probe the Windows errc decoder
  //   test the first entry in the decoder table:
  ec = error_code( ERROR_ACCESS_DENIED, system_category() );
  BOOST_TEST( ec.value() == ERROR_ACCESS_DENIED );
  BOOST_TEST( ec == errc::permission_denied );
  BOOST_TEST( ec.default_error_condition().value() == errc::permission_denied );
  BOOST_TEST( ec.default_error_condition().category() == generic_category() );

  //   test the second entry in the decoder table:
  ec = error_code( ERROR_ALREADY_EXISTS, system_category() );
  BOOST_TEST( ec.value() == ERROR_ALREADY_EXISTS );
  BOOST_TEST( ec == errc::file_exists );
  BOOST_TEST( ec.default_error_condition().value() == errc::file_exists );
  BOOST_TEST( ec.default_error_condition().category() == generic_category() );

  //   test the third entry in the decoder table:
  ec = error_code( ERROR_BAD_UNIT, system_category() );
  BOOST_TEST( ec.value() == ERROR_BAD_UNIT );
  BOOST_TEST( ec == errc::no_such_device );
  BOOST_TEST( ec.default_error_condition().value() == errc::no_such_device );
  BOOST_TEST( ec.default_error_condition().category() == generic_category() );

  //   test the last non-Winsock entry in the decoder table:
  ec = error_code( ERROR_WRITE_PROTECT, system_category() );
  BOOST_TEST( ec.value() == ERROR_WRITE_PROTECT );
  BOOST_TEST( ec == errc::permission_denied );
  BOOST_TEST( ec.default_error_condition().value() == errc::permission_denied );
  BOOST_TEST( ec.default_error_condition().category() == generic_category() );

  //   test the last Winsock entry in the decoder table:
  ec = error_code( WSAEWOULDBLOCK, system_category() );
  BOOST_TEST( ec.value() == WSAEWOULDBLOCK );
  BOOST_TEST( ec == errc::operation_would_block );
  BOOST_TEST( ec.default_error_condition().value() == errc::operation_would_block );
  BOOST_TEST( ec.default_error_condition().category() == generic_category() );

  //   test not-in-table condition:
  ec = error_code( 1234567890, system_category() );
  BOOST_TEST( ec.value() == 1234567890 );
  BOOST_TEST( ec.default_error_condition().value() == 1234567890 );
  BOOST_TEST( ec.default_error_condition().category() == system_category() );

#else // POSIX

  std::cout << "POSIX tests...\n";
  ec = error_code( EACCES, system_category() );
  BOOST_TEST( ec == error_code( errc::permission_denied, system_category() ) );
  BOOST_TEST( error_code( errc::permission_denied, system_category() ) == ec );
  BOOST_TEST( ec == errc::permission_denied );
  BOOST_TEST( errc::permission_denied == ec );
  BOOST_TEST( ec.default_error_condition().value() == errc::permission_denied );
  BOOST_TEST( ec.default_error_condition().category() == generic_category() );

# ifdef __CYGWIN__

  std::cout << "Cygwin tests...\n";
  ec = cygwin_error::no_package;
  BOOST_TEST( ec == cygwin_error::no_package );
  BOOST_TEST( ec == error_code( ENOPKG, system_category() ) );
  BOOST_TEST( ec == error_code( cygwin_error::no_package, system_category() ) );
  BOOST_TEST( ec.default_error_condition().category() == system_category() );

# elif defined(linux) || defined(__linux) || defined(__linux__)

  std::cout << "Linux tests...\n";
  ec = linux_error::dot_dot_error;
  BOOST_TEST( ec == linux_error::dot_dot_error );
  BOOST_TEST( ec == error_code( EDOTDOT, system_category() ) );
  BOOST_TEST( ec == error_code( linux_error::dot_dot_error, system_category() ) );
  BOOST_TEST( ec.default_error_condition().category() == system_category() );

# endif

#endif
  
  return ::boost::report_errors();
}


