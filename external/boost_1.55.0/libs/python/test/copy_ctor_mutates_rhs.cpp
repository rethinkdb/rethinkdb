// Copyright David Abrahams 2003.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/python/detail/copy_ctor_mutates_rhs.hpp>
#include <boost/static_assert.hpp>
#include <memory>
#include <string>

struct foo
{
    operator std::auto_ptr<int>&() const;
};

int main()
{
    using namespace boost::python::detail;
    BOOST_STATIC_ASSERT(!copy_ctor_mutates_rhs<int>::value);
    BOOST_STATIC_ASSERT(copy_ctor_mutates_rhs<std::auto_ptr<int> >::value);
    BOOST_STATIC_ASSERT(!copy_ctor_mutates_rhs<std::string>::value);
    BOOST_STATIC_ASSERT(!copy_ctor_mutates_rhs<foo>::value);
    return 0;
}
