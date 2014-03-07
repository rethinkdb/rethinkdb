// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2012 Bruno Lalande, Paris, France.
// Copyright (c) 2012 Mateusz Loskot, London, UK.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <geometry_test_common.hpp>

#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/util/calculation_type.hpp>

template <typename G1, typename G2>
inline std::string helper()
{
    std::string result;
    result += typeid(typename bg::coordinate_type<G1>::type).name();
    result += "/";
    result += typeid(typename bg::coordinate_type<G2>::type).name();
    return result;
}

template <typename G1, typename G2, typename G3>
inline std::string helper3()
{
    std::string result;
    result += typeid(typename bg::coordinate_type<G1>::type).name();
    result += "/";
    result += typeid(typename bg::coordinate_type<G2>::type).name();
    result += "/";
    result += typeid(typename bg::coordinate_type<G3>::type).name();
    return result;
}

template
<
    typename G1,
    typename G2,
    typename DefaultFP,
    typename DefaultInt,
    typename ExpectedType
>
void test()
{
    typedef typename bg::util::calculation_type::geometric::binary
        <
            G1,
            G2,
            void,
            DefaultFP,
            DefaultInt
        >::type type;

    std::string const caption = helper<G1, G2>();

    BOOST_CHECK_MESSAGE((boost::is_same<type, ExpectedType>::type::value),
        "Failure, types do not agree;"
            << " input: " << caption
            << " defaults: " << typeid(DefaultFP).name()
                << "/" << typeid(DefaultInt).name()
            << " expected: " << typeid(ExpectedType).name()
            << " detected: " << typeid(type).name()
        );
}

template
<
    typename G1,
    typename G2,
    typename CalculationType,
    typename ExpectedType
>
void test_with_calculation_type()
{
    typedef typename bg::util::calculation_type::geometric::binary
        <
            G1,
            G2,
            CalculationType,
            double,
            int
        >::type type;

    std::string const caption = helper<G1, G2>();

    BOOST_CHECK_MESSAGE((boost::is_same<type, ExpectedType>::type::value),
        "Failure, types do not agree;"
            << " input: " << caption
            << " calculation type: " << typeid(CalculationType).name()
            << " expected: " << typeid(ExpectedType).name()
            << " detected: " << typeid(type).name()
        );
}

template
<
    typename Geometry,
    typename DefaultFP,
    typename DefaultInt,
    typename ExpectedType
>
void test_unary()
{
    typedef typename bg::util::calculation_type::geometric::unary
        <
            Geometry,
            void,
            DefaultFP,
            DefaultInt
        >::type type;

    BOOST_CHECK_MESSAGE((boost::is_same<type, ExpectedType>::type::value),
        "Failure, types do not agree;"
            << " input: " << typeid(typename bg::coordinate_type<Geometry>::type).name()
            << " defaults: " << typeid(DefaultFP).name()
                << "/" << typeid(DefaultInt).name()
            << " expected: " << typeid(ExpectedType).name()
            << " detected: " << typeid(type).name()
        );
}


template
<
    typename G1,
    typename G2,
    typename G3,
    typename DefaultFP,
    typename DefaultInt,
    typename ExpectedType
>
void test_ternary()
{
    typedef typename bg::util::calculation_type::geometric::ternary
        <
            G1,
            G2,
            G3,
            void,
            DefaultFP,
            DefaultInt
        >::type type;

    std::string const caption = helper3<G1, G2, G3>();

    BOOST_CHECK_MESSAGE((boost::is_same<type, ExpectedType>::type::value),
        "Failure, types do not agree;"
            << " input: " << caption
            << " defaults: " << typeid(DefaultFP).name()
                << "/" << typeid(DefaultInt).name()
            << " expected: " << typeid(ExpectedType).name()
            << " detected: " << typeid(type).name()
        );
}


struct user_defined {};

int test_main(int, char* [])
{
    using namespace boost::geometry;
    typedef model::point<double, 2, cs::cartesian> d;
    typedef model::point<float, 2, cs::cartesian> f;
    typedef model::point<int, 2, cs::cartesian> i;
    typedef model::point<char, 2, cs::cartesian> c;
    typedef model::point<short int, 2, cs::cartesian> s;
    typedef model::point<boost::long_long_type, 2, cs::cartesian> ll;
    typedef model::point<user_defined, 2, cs::cartesian> u;

    // Calculation type "void" so 
    test<f, f, double, int, double>();
    test<d, d, double, int, double>();
    test<f, d, double, int, double>();

    // FP/int mixed
    test<i, f, double, int, double>();

    // integers
    test<i, i, double, int, int>();
    test<c, i, double, int, int>();
    test<c, c, double, char, char>();
    test<c, c, double, int, int>();
    test<i, i, double, boost::long_long_type, boost::long_long_type>();

    // Even if we specify "int" as default-calculation-type, it should never go downwards.
    // So it will select "long long"
    test<ll, ll, double, int, boost::long_long_type>();

    // user defined
    test<u, i, double, char, user_defined>();
    test<u, d, double, double, user_defined>();

    test_with_calculation_type<i, i, double, double>();
    test_with_calculation_type<u, u, double, double>();

    test_unary<i, double, int, int>();
    test_unary<u, double, double, user_defined>();
    test_ternary<u, u, u, double, double, user_defined>();

    return 0;
}
