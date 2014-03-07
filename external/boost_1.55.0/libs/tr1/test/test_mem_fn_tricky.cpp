//  (C) Copyright John Maddock 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifdef TEST_STD_HEADERS
#include <functional>
#else
#include <boost/tr1/functional.hpp>
#endif

#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include "verify_functor.hpp"

struct expected_result{};

struct test
{
   expected_result field;
   expected_result nullary();
   expected_result nullary_c()const;
   expected_result nullary_v()volatile;
   expected_result nullary_cv()const volatile;
   expected_result unary(int);
   expected_result unary_c(int)const;
   expected_result unary_v(int)volatile;
   expected_result unary_cv(int)const volatile;
};

int main()
{
   verify_field_functor(std::tr1::mem_fn(&test::field), std::unary_function<test const*, const expected_result&>());
   verify_field_functor(std::tr1::mem_fn(&test::field), std::unary_function<test*, expected_result&>());
   verify_unary_functor(std::tr1::mem_fn(&test::nullary), std::unary_function<test*, expected_result>());
   verify_unary_functor(std::tr1::mem_fn(&test::nullary_c), std::unary_function<test const*, expected_result>());
   verify_unary_functor(std::tr1::mem_fn(&test::nullary_v), std::unary_function<test volatile*, expected_result>());
   verify_unary_functor(std::tr1::mem_fn(&test::nullary_cv), std::unary_function<test const volatile*, expected_result>());

   verify_binary_functor(std::tr1::mem_fn(&test::unary), std::binary_function<test*, int, expected_result>());
   verify_binary_functor(std::tr1::mem_fn(&test::unary_c), std::binary_function<test const*, int, expected_result>());
   verify_binary_functor(std::tr1::mem_fn(&test::unary_v), std::binary_function<test volatile*, int, expected_result>());
   verify_binary_functor(std::tr1::mem_fn(&test::unary_cv), std::binary_function<test const volatile*, int, expected_result>());
   return 0;
}

