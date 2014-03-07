//  (C) Copyright John Maddock 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Verify that the headers can be included, even if their contents
// aren't neccessarily usable by broken compilers.

#ifdef TEST_STD_HEADERS

#include <memory>
#include <array>
#include <complex>
#include <functional>
#include <random>
#include <regex>
#include <tuple>
#include <utility>
#include <type_traits>

#else

#include <boost/tr1/array.hpp>
#include <boost/tr1/complex.hpp>
#include <boost/tr1/functional.hpp>
#include <boost/tr1/memory.hpp>
#include <boost/tr1/random.hpp>
#include <boost/tr1/regex.hpp>
#include <boost/tr1/tuple.hpp>
#include <boost/tr1/type_traits.hpp>
#include <boost/tr1/utility.hpp>

#endif





