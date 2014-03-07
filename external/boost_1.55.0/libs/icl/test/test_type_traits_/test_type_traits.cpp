/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#define BOOST_TEST_MODULE icl::test_type_traits unit test

#include <libs/icl/test/disable_test_warnings.hpp>
#include <limits>
#include <complex>
#include <string>
#include <vector>
#include <set>
#include <boost/mpl/list.hpp>
#include "../unit_test_unwarned.hpp"


// interval instance types
#include "../test_type_lists.hpp"
#include "../test_value_maker.hpp"

#include <boost/type_traits/is_same.hpp>

#include <boost/icl/rational.hpp>
#include <boost/icl/interval_map.hpp>
#include <boost/icl/map.hpp>
#include "../test_laws.hpp"

using namespace std;
using namespace boost;
using namespace unit_test;
using namespace boost::icl;

void digits_of_numeric_types()
{
    cout << "--- limits ---\n";
    cout << "max<char>    = " << (std::numeric_limits<char>::max)() << endl;
    cout << "max<char>    = " << static_cast<int>((std::numeric_limits<char>::max)()) << endl;
    cout << "-----------------------------------\n";
    cout << "digits<char>    = " << std::numeric_limits<char>::digits << endl;
    cout << "digits<short>   = " << std::numeric_limits<short>::digits << endl;
    cout << "digits<float>   = " << std::numeric_limits<float>::digits << endl;
    cout << "digits<double>  = " << std::numeric_limits<double>::digits << endl;
    cout << "digits<complex<double>> = " << std::numeric_limits<std::complex<double> >::digits << endl;
    cout << "digits<string>  = " << std::numeric_limits<std::string>::digits << endl;
}

BOOST_AUTO_TEST_CASE(test_icl_infinity)
{
    BOOST_CHECK_EQUAL(icl::infinity<int>::value(), (std::numeric_limits<int>::max)());
    BOOST_CHECK(0 != icl::infinity<int>::value());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_is_continuous_type_T, T, continuous_types)
{
    BOOST_CHECK(is_continuous<T>::value);
    BOOST_CHECK(!is_discrete<T>::value);
}

BOOST_AUTO_TEST_CASE(test_is_continuous_type)
{
    BOOST_CHECK(is_continuous<std::vector<int> >::value);
    BOOST_CHECK(!is_discrete<std::vector<int> >::value);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_is_discrete_type_T, T, discrete_types)
{
    BOOST_CHECK(is_discrete<T>::value);
    BOOST_CHECK(!is_continuous<T>::value);
}

BOOST_AUTO_TEST_CASE(test_is_discrete_type)
{
    BOOST_CHECK(is_discrete<boost::gregorian::date>::value);
    BOOST_CHECK(!is_continuous<boost::gregorian::date>::value);
}

BOOST_AUTO_TEST_CASE(test_is_key_container_of)
{
    BOOST_CHECK((!is_key_container_of<int, icl::map<int,int> >::value));
    BOOST_CHECK((!is_key_container_of<std::pair<int,int> , icl::map<int,int> >::value));
    BOOST_CHECK(( is_key_container_of<std::set<int>,       std::set<int>     >::value));
    BOOST_CHECK(( is_key_container_of<ICL_IMPL_SPACE::set<int>,       icl::map<int,int> >::value));
    BOOST_CHECK(( is_key_container_of<icl::map<int,int>,   icl::map<int,int> >::value));
}

BOOST_AUTO_TEST_CASE(test_is_set_4_std_set)
{
    BOOST_CHECK( (is_set<std::set<int> >::value) );
    BOOST_CHECK( (is_element_set<std::set<int> >::value) );
    BOOST_CHECK( (!is_map<std::set<int> >::value) );

    BOOST_CHECK( (is_set<ICL_IMPL_SPACE::set<int> >::value) );
    BOOST_CHECK( (is_element_set<ICL_IMPL_SPACE::set<int> >::value) );
    BOOST_CHECK( (!is_map<ICL_IMPL_SPACE::set<int> >::value) );
}

BOOST_AUTO_TEST_CASE(test_miscellaneous_type_traits)
{
    typedef interval_set<int> IntervalSetT;
    typedef icl::map<int,int> MapII;

    BOOST_CHECK(has_codomain_type<MapII>::value);
    BOOST_CHECK((boost::is_same<MapII::codomain_type, int>::value));

    BOOST_CHECK((is_map<MapII>::value));
    BOOST_CHECK((is_icl_container<MapII>::value));

    BOOST_CHECK((is_fragment_of<IntervalSetT::element_type, IntervalSetT>::value));
    BOOST_CHECK((is_fragment_of<IntervalSetT::segment_type, IntervalSetT>::value));
    BOOST_CHECK((!is_fragment_of<double, IntervalSetT>::value));

    BOOST_CHECK((boost::detail::is_incrementable<int>::value));
    BOOST_CHECK((boost::detail::is_incrementable<double>::value));
    BOOST_CHECK((!boost::detail::is_incrementable<std::string>::value));

    BOOST_CHECK((boost::is_floating_point<long double>::value));
    BOOST_CHECK((boost::is_floating_point<double>::value));
    BOOST_CHECK((boost::is_floating_point<float>::value));

    BOOST_CHECK( (boost::is_same<key_type_of<std::set<int> >::type, int>::value) );
    BOOST_CHECK( (boost::is_same<value_type_of<std::set<int> >::type, int>::value) );

    BOOST_CHECK(  is_std_set<std::set<int> >::value);
    BOOST_CHECK( !is_std_set<interval_set<int> >::value);
    BOOST_CHECK((!is_std_set<std::map<int,int> >::value));
    BOOST_CHECK( is_element_set<std::set<int> >::value);
    BOOST_CHECK( !is_interval_set<std::set<int> >::value);
    BOOST_CHECK( !is_interval_set<std::set<int> >::value);
}

