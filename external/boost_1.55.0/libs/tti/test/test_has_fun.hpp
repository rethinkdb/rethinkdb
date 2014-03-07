
//  (C) Copyright Edward Diener 2012
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_TEST_HAS_FUNCTION_HPP)
#define BOOST_TEST_HAS_FUNCTION_HPP

#include "test_structs.hpp"
#include <boost/tti/has_function.hpp>

BOOST_TTI_HAS_FUNCTION(VoidFunction)
BOOST_TTI_TRAIT_HAS_FUNCTION(TheTIntFunction,TIntFunction)
BOOST_TTI_TRAIT_HAS_FUNCTION(FunctionReturningInt,IntFunction)
BOOST_TTI_HAS_FUNCTION(aFunction)
BOOST_TTI_TRAIT_HAS_FUNCTION(AnotherIntFunction,anotherFunction)
BOOST_TTI_TRAIT_HAS_FUNCTION(Pickedname,SomeStaticFunction)
BOOST_TTI_HAS_FUNCTION(sFunction)
BOOST_TTI_HAS_FUNCTION(someFunctionMember)
BOOST_TTI_TRAIT_HAS_FUNCTION(HaveTheSIntFunction,SIntFunction)
BOOST_TTI_HAS_FUNCTION(TSFunction)

#endif // BOOST_TEST_HAS_FUNCTION_HPP
