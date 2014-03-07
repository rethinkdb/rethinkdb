// Copyright David Abrahams 2003.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/implicit_cast.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/type.hpp>
using boost::implicit_cast;
using boost::type;

template <class T>
type<T> check_return(T) { return type<T>(); }

struct foo
{
    foo(char const*) {}
    operator long() const { return 0; }
};

typedef type<long> long_type;
typedef type<foo> foo_type;

int main()
{
    type<long> x = check_return(boost::implicit_cast<long>(1));
    BOOST_TEST(boost::implicit_cast<long>(1) == 1L);

    type<foo> f = check_return(boost::implicit_cast<foo>("hello"));
    type<long> z = check_return(boost::implicit_cast<long>(foo("hello")));

    // warning supression:
    (void)x;
    (void)f;
    (void)z;

    return boost::report_errors();
}
