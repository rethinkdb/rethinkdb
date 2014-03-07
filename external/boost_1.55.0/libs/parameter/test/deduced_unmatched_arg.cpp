// Copyright Daniel Wallin 2006. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/parameter/parameters.hpp>
#include <boost/parameter/name.hpp>

namespace parameter = boost::parameter;
namespace mpl = boost::mpl;

BOOST_PARAMETER_NAME(x)

int main()
{
    using namespace parameter;
    using boost::is_convertible;
    using mpl::_;

    parameters<
        optional<
            deduced<tag::x>, is_convertible<_,int>
        >
    >()("foo");
}

