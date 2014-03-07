///////////////////////////////////////////////////////////////////////////////
// cpp-next_bug.hpp
//
//  Copyright 2012 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <vector>
#include <boost/proto/proto.hpp>
#include <boost/test/unit_test.hpp>
namespace mpl = boost::mpl;
namespace proto = boost::proto;
using proto::_;

namespace linear_algebra
{
    // A trait that returns true only for std::vector
    template<typename T>
    struct is_std_vector
      : mpl::false_
    {};

    template<typename T, typename A>
    struct is_std_vector<std::vector<T, A> >
      : mpl::true_
    {};

    // A type used as a domain for linear algebra expressions
    struct linear_algebra_domain
      : proto::domain<>
    {};

    // Define all the operator overloads for combining std::vectors
    BOOST_PROTO_DEFINE_OPERATORS(is_std_vector, linear_algebra_domain)

    // Take any expression and turn each node
    // into a subscript expression, using the
    // state as the RHS.
    struct Distribute
      : proto::or_<
            proto::when<proto::terminal<_>, proto::_make_subscript(_, proto::_state)>
          , proto::plus<Distribute, Distribute>
        >
    {};

    struct Optimize
      : proto::or_<
            proto::when<
                proto::subscript<Distribute, proto::terminal<_> >,
                Distribute(proto::_left, proto::_right)
            >
          , proto::plus<Optimize, Optimize>
          , proto::terminal<_>
        >
    {};
}

static const int celems = 4;
static int const value[celems] = {1,2,3,4};
std::vector<int> A(value, value+celems), B(A);

void test1()
{
    using namespace linear_algebra;
    proto::_default<> eval;
    BOOST_CHECK_EQUAL(8, eval(Optimize()((A + B)[3])));
}

using namespace boost::unit_test;
///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("test for a problem reported on the cpp-next.com blog");

    test->add(BOOST_TEST_CASE(&test1));

    return test;
}
