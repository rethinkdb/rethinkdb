
// Copyright 2011 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This tests if container forwarding is correctly disabled. If it isn't
// disabled it causes a compile error (which causes the test to pass).
// If it is disabled it tries container forwarding. If it doesn't work
// then there will be a compile error, indicating that it is correctly
// disabled. But if there isn't a compile error that indicates that
// container forwarding might work.
//
// Since this test only tries std::vector, it might get it wrong but I didn't
// want it to fail because of some incompatibility with a trickier class.

#define BOOST_DETAIL_TEST_CONFIG_ONLY
#include <boost/detail/container_fwd.hpp>

#if !defined(BOOST_DETAIL_NO_CONTAINER_FWD)
#error "Failing in order to pass test"
#else
#define BOOST_DETAIL_TEST_FORCE_CONTAINER_FWD

#undef BOOST_DETAIL_CONTAINER_FWD_HPP
#undef BOOST_DETAIL_TEST_CONFIG_ONLY

#include <boost/detail/container_fwd.hpp>

template <class T, class Allocator>
void test(std::vector<T, Allocator> const&)
{
}

#include <vector>

int main ()
{
    std::vector<int> x;
    test(x);
}

#endif

