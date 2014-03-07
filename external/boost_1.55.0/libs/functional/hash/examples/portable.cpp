
// Copyright 2005-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/functional/hash.hpp>
#include <cassert>

// This example illustrates how to customise boost::hash portably, so that
// it'll work on both compilers that don't implement argument dependent lookup
// and compilers that implement strict two-phase template instantiation.

namespace foo
{
    template <class T>
    class custom_type
    {
        T value;
    public:
        custom_type(T x) : value(x) {}

        std::size_t hash() const
        {
            boost::hash<T> hasher;
            return hasher(value);
        }
    };
}

#ifdef BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP
namespace boost
#else
namespace foo
#endif
{
    template <class T>
    std::size_t hash_value(foo::custom_type<T> x)
    {
        return x.hash();
    }
}

int main()
{
    foo::custom_type<int> x(1), y(2), z(1);

    boost::hash<foo::custom_type<int> > hasher;

    assert(hasher(x) == hasher(x));
    assert(hasher(x) != hasher(y));
    assert(hasher(x) == hasher(z));

    return 0;
}
