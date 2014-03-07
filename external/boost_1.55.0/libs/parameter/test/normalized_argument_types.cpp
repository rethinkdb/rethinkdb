// Copyright Daniel Wallin 2006. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/parameter.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <cassert>

struct count_instances
{
    count_instances()
    {
        ++count;
    }

    count_instances(count_instances const&)
    {
        ++count;
    }

    template <class T>
    count_instances(T const&)
    {
        ++count;
    }

    ~count_instances()
    {
        --count;
    }

    static std::size_t count;
};

std::size_t count_instances::count = 0;

BOOST_PARAMETER_NAME(x)
BOOST_PARAMETER_NAME(y)

BOOST_PARAMETER_FUNCTION((int), f, tag,
    (required
       (x, (int))
       (y, (int))
    )
)
{
    BOOST_MPL_ASSERT((boost::is_same<x_type,int>));
    BOOST_MPL_ASSERT((boost::is_same<y_type,int>));
    return 0;
}

BOOST_PARAMETER_FUNCTION((int), g, tag,
    (required
       (x, (count_instances))
    )
)
{
    BOOST_MPL_ASSERT((boost::is_same<x_type,count_instances>));
    assert(count_instances::count > 0);
    return 0;
}

BOOST_PARAMETER_FUNCTION((int), h, tag,
    (required
       (x, (count_instances const&))
    )
)
{
    BOOST_MPL_ASSERT((boost::is_same<x_type,count_instances const>));
    assert(count_instances::count == 1);
    return 0;
}

int main()
{
    f(1, 2);
    f(1., 2.f);
    f(1U, 2L);

    g(0);

    h(0);
}

