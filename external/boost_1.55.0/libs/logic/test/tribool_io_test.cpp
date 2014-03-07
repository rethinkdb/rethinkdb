// Copyright Douglas Gregor 2002-2004. Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/logic/tribool.hpp>
#include <boost/logic/tribool_io.hpp>
#include <boost/test/minimal.hpp>
#include <sstream>
#include <string>
#include <iostream>
#include <ios>     // for std::boolalpha

#ifndef BOOST_NO_STD_LOCALE
#  include <locale>
#endif

int test_main(int, char*[])
{
  using namespace boost::logic;

  tribool x;

  // Check tribool output
  std::ostringstream out;

  // Output false (noboolalpha)
  out.str(std::string());
  x = false;
  out << x;
  std::cout << "Output false (noboolalpha): " << out.str() << std::endl;
  BOOST_CHECK(out.str() == "0");

  // Output true (noboolalpha)
  out.str(std::string());
  x = true;
  out << x;
  std::cout << "Output true (noboolalpha): " << out.str() << std::endl;
  BOOST_CHECK(out.str() == "1");

  // Output indeterminate (noboolalpha)
  out.str(std::string());
  x = indeterminate;
  out << x;
  std::cout << "Output indeterminate (noboolalpha): " << out.str()
            << std::endl;
  BOOST_CHECK(out.str() == "2");

  // Output indeterminate (noboolalpha)
  out.str(std::string());
  out << indeterminate;
  std::cout << "Output indeterminate (noboolalpha): " << out.str()
            << std::endl;
  BOOST_CHECK(out.str() == "2");

#ifndef BOOST_NO_STD_LOCALE
  const std::numpunct<char>& punct =
    BOOST_USE_FACET(std::numpunct<char>, out.getloc());

  // Output false (boolalpha)
  out.str(std::string());
  x = false;
  out << std::boolalpha << x;
  std::cout << "Output false (boolalpha): " << out.str() << std::endl;
  BOOST_CHECK(out.str() == punct.falsename());

  // Output true (boolalpha)
  out.str(std::string());
  x = true;
  out << std::boolalpha << x;
  std::cout << "Output true (boolalpha): " << out.str() << std::endl;

  BOOST_CHECK(out.str() == punct.truename());

  // Output indeterminate (boolalpha - default name)
  out.str(std::string());
  x = indeterminate;
  out << std::boolalpha << x;
  std::cout << "Output indeterminate (boolalpha - default name): " << out.str()
            << std::endl;
  BOOST_CHECK(out.str() == "indeterminate");

  // Output indeterminate (boolalpha - default name)
  out.str(std::string());
  out << std::boolalpha << indeterminate;
  std::cout << "Output indeterminate (boolalpha - default name): " << out.str()
            << std::endl;
  BOOST_CHECK(out.str() == "indeterminate");

#  if BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1)
  // No template constructors, so we can't build the test locale
#  else
  // Give indeterminate a new name, and output it via boolalpha
  std::locale global;
  std::locale test_locale(global, new indeterminate_name<char>("maybe"));
  out.imbue(test_locale);
  out.str(std::string());
  out << std::boolalpha << x;
  std::cout << "Output indeterminate (boolalpha - \"maybe\"): " << out.str()
            << std::endl;
  BOOST_CHECK(out.str() == "maybe");
#  endif
#endif // ! BOOST_NO_STD_LOCALE

  // Checking tribool input

  // Input false (noboolalpha)
  {
    std::istringstream in("0");
    std::cout << "Input \"0\" (checks for false)" << std::endl;
    in >> x;
    BOOST_CHECK(x == false);
  }

  // Input true (noboolalpha)
  {
    std::istringstream in("1");
    std::cout << "Input \"1\" (checks for true)" << std::endl;
    in >> x;
    BOOST_CHECK(x == true);
  }

  // Input false (noboolalpha)
  {
    std::istringstream in("2");
    std::cout << "Input \"2\" (checks for indeterminate)" << std::endl;
    in >> x;
    BOOST_CHECK(indeterminate(x));
  }

  // Input bad number (noboolalpha)
  {
    std::istringstream in("3");
    std::cout << "Input \"3\" (checks for failure)" << std::endl;
    BOOST_CHECK(!(in >> x));
  }

  // Input false (boolalpha)
  {
    std::istringstream in("false");
    std::cout << "Input \"false\" (checks for false)" << std::endl;
    in >> std::boolalpha >> x;
    BOOST_CHECK(x == false);
  }

  // Input true (boolalpha)
  {
    std::istringstream in("true");
    std::cout << "Input \"true\" (checks for true)" << std::endl;
    in >> std::boolalpha >> x;
    BOOST_CHECK(x == true);
  }

  // Input indeterminate (boolalpha)
  {
    std::istringstream in("indeterminate");
    std::cout << "Input \"indeterminate\" (checks for indeterminate)"
              << std::endl;
    in >> std::boolalpha >> x;
    BOOST_CHECK(indeterminate(x));
  }

  // Input bad string (boolalpha)
  {
    std::istringstream in("bad");
    std::cout << "Input \"bad\" (checks for failure)"
              << std::endl;
    BOOST_CHECK(!(in >> std::boolalpha >> x));
  }

#if BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1)
  // No template constructors, so we can't build the test locale
#elif !defined(BOOST_NO_STD_LOCALE)

  // Input indeterminate named "maybe" (boolalpha)
  {
    std::istringstream in("maybe");
    in.imbue(test_locale);
    std::cout << "Input \"maybe\" (checks for indeterminate, uses locales)"
              << std::endl;
    in >> std::boolalpha >> x;
    BOOST_CHECK(indeterminate(x));
  }

  // Input indeterminate named "true_or_false" (boolalpha)
  {
    std::locale my_locale(global,
                          new indeterminate_name<char>("true_or_false"));
    std::istringstream in("true_or_false");
    in.imbue(my_locale);
    std::cout << "Input \"true_or_false\" (checks for indeterminate)"
              << std::endl;
    in >> std::boolalpha >> x;
    BOOST_CHECK(indeterminate(x));
  }
#endif

  return 0;
}
