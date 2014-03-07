// Copyright (c) 2011
// Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "boost/test/minimal.hpp"
#include "boost/variant/detail/hash_variant.hpp"
#include "boost/variant.hpp"
#include "boost/functional/hash/hash.hpp"

void run()
{
    typedef boost::variant<bool, int, unsigned int, double> variant_type;
		boost::hash<variant_type> hasher;
		
		variant_type bool_variant1 = true;
		variant_type bool_variant2 = false;
		variant_type int_variant = 1;
		variant_type float_variant = 1.0;
		variant_type uint_variant = static_cast<unsigned int>(1);

    BOOST_CHECK(hasher(bool_variant1) != hasher(bool_variant2));
    BOOST_CHECK(hasher(bool_variant1) == hasher(bool_variant1));
    BOOST_CHECK(hasher(int_variant) != hasher(uint_variant));
    BOOST_CHECK(hasher(float_variant) != hasher(uint_variant));
}

int test_main(int , char* [])
{
   run();
   return 0;
}
