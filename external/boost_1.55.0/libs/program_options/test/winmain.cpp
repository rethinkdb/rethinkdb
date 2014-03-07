// Copyright Vladimir Prus 2002-2004.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#if defined(_WIN32)
#include <string>
#include <vector>
#include <cctype>
#include <iostream>
#include <stdlib.h>

using namespace std;

#include <boost/program_options/parsers.hpp>
using namespace boost::program_options;

void check_equal(const std::vector<string>& actual, char **expected, int n)
{
    if (actual.size() != n)
    {
        std::cerr << "Size mismatch between expected and actual data\n";
        abort();
    }
    for (int i = 0; i < n; ++i)
    {
        if (actual[i] != expected[i])
        {
            std::cerr << "Unexpected content\n";
            abort();
        }
    }
}

#include <boost/preprocessor/cat.hpp>

void test_winmain()
{

#define C ,
#define TEST(input, expected) \
    char* BOOST_PP_CAT(e, __LINE__)[] = expected;\
    vector<string> BOOST_PP_CAT(v, __LINE__) = split_winmain(input);\
    check_equal(BOOST_PP_CAT(v, __LINE__), BOOST_PP_CAT(e, __LINE__),\
                sizeof(BOOST_PP_CAT(e, __LINE__))/sizeof(char*));    

// The following expectations were obtained in Win2000 shell:
    TEST("1 ", {"1"});
    TEST("1\"2\" ", {"12"});
    TEST("1\"2  ", {"12  "});
    TEST("1\"\\\"2\" ", {"1\"2"});
    TEST("\"1\" \"2\" ", {"1" C "2"});
    TEST("1\\\" ", {"1\""});
    TEST("1\\\\\" ", {"1\\ "});
    TEST("1\\\\\\\" ", {"1\\\""});
    TEST("1\\\\\\\\\" ", {"1\\\\ "});

    TEST("1\" 1 ", {"1 1 "});
    TEST("1\\\" 1 ", {"1\"" C "1"});
    TEST("1\\1 ", {"1\\1"});
    TEST("1\\\\1 ", {"1\\\\1"});    
}

int main(int, char*[])
{
    test_winmain();
    return 0;
}
#else
int main(int, char*[])
{
    return 0;
}
#endif
