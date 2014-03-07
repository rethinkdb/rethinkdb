
// Copyright 2005-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This uses std::rand to generate random values for tests.
// Which is not good as different platforms will be running different tests.
// It would be much better to use Boost.Random, but it doesn't
// support all the compilers that I want to test on.

#if !defined(BOOST_UNORDERED_TEST_HELPERS_GENERATORS_HEADER)
#define BOOST_UNORDERED_TEST_HELPERS_GENERATORS_HEADER

#include <string>
#include <utility>
#include <stdexcept>
#include <cstdlib>
#include <boost/type_traits/add_const.hpp>
#include "./fwd.hpp"

namespace test
{
    struct seed_t {
        seed_t(unsigned int x) {
            using namespace std;
            srand(x);
        }
    };

    inline int generate(int const*)
    {
        using namespace std;
        return rand();
    }

    inline char generate(char const*)
    {
        using namespace std;
        return static_cast<char>((rand() >> 1) % (128-32) + 32);
    }

    inline signed char generate(signed char const*)
    {
        using namespace std;
        return static_cast<signed char>(rand());
    }

    inline std::string generate(std::string const*)
    {
        using namespace std;

        char* char_ptr = 0;

        std::string result;

        int length = rand() % 10;
        for(int i = 0; i < length; ++i)
            result += generate(char_ptr);

        return result;
    }

    float generate(float const*)
    {
        using namespace std;
        return (float) rand() / (float) RAND_MAX;
    }
}

#endif
