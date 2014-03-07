///////////////////////////////////////////////////////////////////////////////
// deduce_domain.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Avoid a compile-time check inside the deduce_domain code.
#define BOOST_PROTO_ASSERT_VALID_DOMAIN(DOM) typedef DOM DOM ## _

#include <boost/proto/core.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>

namespace proto = boost::proto;
using proto::_;

struct D0 : proto::domain<>
{
};

struct D1 : proto::domain<proto::default_generator, _, D0>
{
};

struct D2 : proto::domain<proto::default_generator, _, D0>
{
};

struct D3 : proto::domain<>
{
};

struct DD0 : proto::domain<proto::default_generator, _, proto::default_domain>
{
};

struct DD1 : proto::domain<proto::default_generator, _, proto::default_domain>
{
};

struct DD2 : proto::domain<proto::default_generator, _, proto::default_domain>
{
};

struct DD3 : proto::domain<proto::default_generator, _, DD2>
{
};

struct DD4 : proto::domain<proto::default_generator, _, DD2>
{
};

void test1()
{
    using boost::is_same;

    //*
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D0, D0, D0>::type, D0>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<proto::default_domain, D0, D0>::type, D0>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D0, proto::default_domain, D0>::type, D0>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D0, D0, proto::default_domain>::type, D0>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D0, proto::default_domain, proto::default_domain>::type, D0>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<proto::default_domain, D0, proto::default_domain>::type, D0>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<proto::default_domain, proto::default_domain, D0>::type, D0>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<proto::default_domain, proto::default_domain, proto::default_domain>::type, proto::default_domain>));

    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<DD0, D0, D0>::type, D0>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D0, DD0, D0>::type, D0>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D0, D0, DD0>::type, D0>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D0, DD0, DD0>::type, D0>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<DD0, D0, DD0>::type, D0>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<DD0, DD0, D0>::type, D0>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<proto::default_domain, DD0, DD0>::type, DD0>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<DD0, proto::default_domain, DD0>::type, DD0>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<DD0, DD0, proto::default_domain>::type, DD0>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<proto::default_domain, proto::default_domain, DD0>::type, DD0>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<proto::default_domain, DD0, proto::default_domain>::type, DD0>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<DD0, DD0, proto::default_domain>::type, DD0>));

    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D0, D0, D1>::type, D0>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D0, D1, D0>::type, D0>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D0, D1, D1>::type, D0>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D1, D0, D0>::type, D0>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D1, D0, D1>::type, D0>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D1, D1, D0>::type, D0>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D1, D1, D1>::type, D1>));

    // Very tricky to get right
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D2, D2, D1>::type, D0>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D2, D1, D2>::type, D0>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D2, D1, D1>::type, D0>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D1, D2, D2>::type, D0>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D1, D2, D1>::type, D0>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D1, D1, D2>::type, D0>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D1, D1, D1>::type, D1>));

    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D3, D0, D0>::type, proto::detail::not_a_domain>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D0, D3, D0>::type, proto::detail::not_a_domain>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D0, D0, D3>::type, proto::detail::not_a_domain>));

    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D3, D1, D0>::type, proto::detail::not_a_domain>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D3, D0, D1>::type, proto::detail::not_a_domain>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D1, D3, D0>::type, proto::detail::not_a_domain>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D0, D3, D1>::type, proto::detail::not_a_domain>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D0, D1, D3>::type, proto::detail::not_a_domain>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D1, D0, D3>::type, proto::detail::not_a_domain>));

    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D3, D1, D2>::type, proto::detail::not_a_domain>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D3, D2, D1>::type, proto::detail::not_a_domain>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D1, D3, D2>::type, proto::detail::not_a_domain>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D2, D3, D1>::type, proto::detail::not_a_domain>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D2, D1, D3>::type, proto::detail::not_a_domain>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<D1, D2, D3>::type, proto::detail::not_a_domain>));

    // These should be ambiguous.
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<DD1, DD0, DD0>::type, proto::detail::not_a_domain>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<DD0, DD1, DD0>::type, proto::detail::not_a_domain>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<DD0, DD0, DD1>::type, proto::detail::not_a_domain>));

    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<DD3, DD2, DD2>::type, DD2>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<DD2, DD3, DD2>::type, DD2>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<DD2, DD2, DD3>::type, DD2>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<DD3, DD4, DD4>::type, DD2>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<DD4, DD3, DD4>::type, DD2>));
    BOOST_MPL_ASSERT((is_same<proto::detail::common_domain3<DD4, DD4, DD3>::type, DD2>));
    //*/
}

using namespace boost::unit_test;
///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("test deducing domains from sub-domains");

    test->add(BOOST_TEST_CASE(&test1));

    return test;
}
