#ifndef BOOST_PROGRAM_OPTIONS_MINITEST
#define BOOST_PROGRAM_OPTIONS_MINITEST

#include <assert.h>
#include <iostream>
#include <stdlib.h>

#define BOOST_REQUIRE(b) assert(b)
#define BOOST_CHECK(b) assert(b)
#define BOOST_CHECK_EQUAL(a, b) assert(a == b)
#define BOOST_ERROR(description) std::cerr << description; std::cerr << "\n"; abort();
#define BOOST_CHECK_THROW(expression, exception) \
    try \
    { \
        expression; \
        BOOST_ERROR("expected exception not thrown");\
        throw 10; \
    } \
    catch(exception &) \
    { \
    }



#endif
