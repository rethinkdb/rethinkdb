/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#define BOOST_TEST_MODULE icl::misc unit test

#define BOOST_ICL_TEST_CHRONO

#include <libs/icl/test/disable_test_warnings.hpp>
#include <string>
#include <vector>
#include <boost/mpl/list.hpp>
#include "../unit_test_unwarned.hpp"


// interval instance types
#include "../test_type_lists.hpp"
#include "../test_value_maker.hpp"

#include <boost/type_traits/is_same.hpp>
#include <boost/icl/rational.hpp>

#include <boost/icl/detail/interval_morphism.hpp>
#include <boost/icl/interval_map.hpp>
#include <boost/icl/split_interval_set.hpp>
#include "../test_laws.hpp"

using namespace std;
using namespace boost;
using namespace unit_test;
using namespace boost::icl;


BOOST_AUTO_TEST_CASE(test_law_complementarity)
{
    //LAW Inner complementarity: x + between(x) == hull(x)
    //LAW: length(x) + length(between(x)) = length(hull(x))
    typedef interval_map<rational<int>, int> RatioMapT;
    typedef interval_set<rational<int> > RatioSetT;
    typedef RatioSetT::interval_type     IntervalT;
    typedef RatioSetT::element_type      RatT;
    typedef RatioSetT::difference_type   DiffT;

    RatioSetT set_a;
    (((set_a += IntervalT(RatT(0),   RatT(1)  ) )
             -= IntervalT(RatT(1,9), RatT(2,9))  )
             -= IntervalT(RatT(3,9), RatT(4,9))   )
             -= IntervalT(RatT(5,9), RatT(6,9));

    RatioSetT between_a  = RatioSetT(hull(set_a)) - set_a;
    RatioSetT between_a2; 
    between(between_a2, set_a);
    BOOST_CHECK_EQUAL( between_a, between_a2 );

    DiffT len_set_a     = length(set_a);
    DiffT len_between_a = length(between_a);

    //cout << set_a     << " length= " << len_set_a     << endl;
    //cout << between_a << " length= " << len_between_a << endl;

    RatioSetT span_a = set_a + between_a;
    RatioSetT hull_a = RatioSetT(hull(set_a));
    //cout << span_a << endl;

    BOOST_CHECK_EQUAL( span_a, hull_a );
    BOOST_CHECK_EQUAL( len_set_a + len_between_a, length(hull_a) );

    BOOST_CHECK((has_inner_complementarity<RatioSetT,RatioSetT>(set_a)));
    BOOST_CHECK((has_length_complementarity(set_a)));
    BOOST_CHECK((has_length_as_distance<RatioSetT,RatioSetT>(set_a)));
}


BOOST_AUTO_TEST_CASE(test_between)
{
    //LAW: between(a,b) == between(b,a);
    typedef int T;
    typedef interval<T>::type IntervalT;
    typedef interval_set<T>   IntervalSetT;

    IntervalT itv_a = I_D(1,3);
    IntervalT itv_b = I_D(5,7);

    IntervalT beween_a_b = between(itv_a, itv_b);
    IntervalT beween_b_a = between(itv_b, itv_a);

    //cout << beween_a_b << endl;
    //cout << beween_b_a << endl;
    BOOST_CHECK_EQUAL( beween_a_b, beween_b_a );
}


BOOST_AUTO_TEST_CASE(element_iteration)
{
    interval_map<int,int> map_a;
    map_a += make_pair(interval<int>::right_open(0,3),1);
    //cout << map_a << endl;

    //for(interval_map<int,int>::element_iterator elem = elements_begin(map_a);
    //    elem != elements_end(map_a); elem++)
    //    cout << "(" << elem->first << "," << elem->second << ")";
    //cout << "\n-------------------------------------\n";

    std::pair<const int, int> search_pair(2,1);

    //interval_map<int,int>::element_const_iterator found 
    interval_map<int,int>::element_iterator found 
        = std::find(elements_begin(map_a), elements_end(map_a), search_pair);
    // cout << "(" << found->first << "," << found->second << ")\n";
    BOOST_CHECK_EQUAL( found->first,  2 );
    BOOST_CHECK_EQUAL( found->second, 1 );

    // Assignment of an associated value via element_iterator
    const_cast<int&>(found->second) = 2;
    // cout << map_a << endl;
    BOOST_CHECK_EQUAL( map_a.begin()->second, 2 );
}


BOOST_AUTO_TEST_CASE(test_interval_bounds_1)
{
    BOOST_CHECK_EQUAL(left_bracket(interval_bounds::closed()),      "[");
    BOOST_CHECK_EQUAL(left_bracket(interval_bounds::right_open()),  "[");
    BOOST_CHECK_EQUAL(left_bracket(interval_bounds::left_open()),   "(");
    BOOST_CHECK_EQUAL(left_bracket(interval_bounds::open()),        "(");
    BOOST_CHECK_EQUAL(right_bracket(interval_bounds::closed()),     "]");
    BOOST_CHECK_EQUAL(right_bracket(interval_bounds::right_open()), ")");
    BOOST_CHECK_EQUAL(right_bracket(interval_bounds::left_open()),  "]");
    BOOST_CHECK_EQUAL(right_bracket(interval_bounds::open()),       ")");

    continuous_interval<double> a_1 = continuous_interval<double>(-5.0, -2.3, interval_bounds::closed());
    continuous_interval<double> b_1 = continuous_interval<double>(-2.6, 4.0, interval_bounds::closed());

    split_interval_set<double> a, b, a_o_b, b_o_a;
    a_o_b += a_1;
    a_o_b += b_1;

    b_o_a += b_1;
    b_o_a += a_1;

    BOOST_CHECK_EQUAL(a_o_b, b_o_a);

    continuous_interval<double> c_1 = continuous_interval<double>(1.0, 3.0, interval_bounds::closed());
    continuous_interval<double> b_2 = right_subtract(b_1, c_1);

    BOOST_CHECK_EQUAL(b_2.bounds(), interval_bounds::right_open());
    BOOST_CHECK_EQUAL(icl::bounds(b_2), interval_bounds::right_open());

    continuous_interval<double> L0T = continuous_interval<double>(0.0, 0.0, interval_bounds::closed());
    continuous_interval<double> C0T = continuous_interval<double>(0.0, 0.0, interval_bounds::left_open());
    continuous_interval<double> L0D = continuous_interval<double>(0.0, 0.0, interval_bounds::right_open());
    continuous_interval<double> C0D = continuous_interval<double>(0.0, 0.0, interval_bounds::open());

    BOOST_CHECK_EQUAL(icl::is_empty(L0T), false);
    BOOST_CHECK_EQUAL(icl::is_empty(C0T), true);
    BOOST_CHECK_EQUAL(icl::is_empty(L0D), true);
    BOOST_CHECK_EQUAL(icl::is_empty(C0D), true);


    continuous_interval<double> L0_1T = continuous_interval<double>(0.0, 1.0, interval_bounds::closed());
    continuous_interval<double> L1_2T = continuous_interval<double>(1.0, 2.0, interval_bounds::closed());
    BOOST_CHECK_EQUAL(icl::exclusive_less(L0_1T, L1_2T), false);
    BOOST_CHECK_EQUAL(icl::inner_bounds(L0_1T, L1_2T) == interval_bounds::open(), true);

    continuous_interval<double> L0_1D = continuous_interval<double>(0.0, 1.0, interval_bounds::right_open());
    BOOST_CHECK_EQUAL(icl::exclusive_less(L0_1D, L1_2T), true);
    BOOST_CHECK_EQUAL(icl::inner_bounds(L0_1D, L1_2T) == interval_bounds::right_open(), true);

    continuous_interval<double> C1_2T = continuous_interval<double>(1.0, 2.0, interval_bounds::left_open());
    BOOST_CHECK_EQUAL(icl::exclusive_less(L0_1T, C1_2T), true);
    BOOST_CHECK_EQUAL(icl::inner_bounds(L0_1T, C1_2T) == interval_bounds::left_open(), true);

    BOOST_CHECK_EQUAL(icl::exclusive_less(L0_1D, C1_2T), true);
    BOOST_CHECK_EQUAL(icl::inner_bounds(L0_1D, C1_2T) == interval_bounds::closed(), true);

    BOOST_CHECK_EQUAL(static_cast<int>(icl::right(L0_1T.bounds()).bits()), 1);
    BOOST_CHECK_EQUAL(static_cast<int>(icl::right(L0_1D.bounds()).bits()), 0);

    BOOST_CHECK_EQUAL(icl::right_bounds(L0_1D, L0_1T), interval_bounds::left_open());
}


BOOST_AUTO_TEST_CASE(test_infinities)
{
    BOOST_CHECK(( has_std_infinity<double>::value));
    BOOST_CHECK((!has_std_infinity<int>::value));

    BOOST_CHECK(( has_max_infinity<int>::value ));
    BOOST_CHECK((!has_max_infinity<double>::value ));

    //--------------------------------------------------------------------------
    BOOST_CHECK_EQUAL( numeric_infinity<double>::value(),  (std::numeric_limits<double>::infinity)() );
    BOOST_CHECK_EQUAL( numeric_infinity<int>::value(),     (std::numeric_limits<int>::max)() );
    BOOST_CHECK_EQUAL( numeric_infinity<std::string>::value(), std::string() );

    //--------------------------------------------------------------------------
    BOOST_CHECK_EQUAL( icl::infinity<double>::value(),  (std::numeric_limits<double>::infinity)() );
    BOOST_CHECK_EQUAL( icl::infinity<int>::value(),     (std::numeric_limits<int>::max)() );
    BOOST_CHECK_EQUAL( icl::infinity<std::string>::value(), icl::identity_element<std::string>::value() );

    //--------------------------------------------------------------------------
    BOOST_CHECK_EQUAL( icl::infinity<chrono::duration<double> >::value()
                     , chrono::duration<double>((std::numeric_limits<double>::infinity)()) );
    BOOST_CHECK_EQUAL( icl::infinity<chrono::duration<int> >::value()
                     , chrono::duration<int>((std::numeric_limits<int>::max)()) );
}


BOOST_AUTO_TEST_CASE(test_difference_types)
{
    BOOST_CHECK(( boost::is_same< int,            difference_type_of<int>::type >::value ));
    BOOST_CHECK(( boost::is_same< double,         difference_type_of<double>::type >::value ));
    BOOST_CHECK(( boost::is_same< std::ptrdiff_t, difference_type_of<int*>::type >::value ));

    BOOST_CHECK(( has_difference_type<std::string>::value ));
    BOOST_CHECK(( boost::is_same< std::string::difference_type, difference_type_of<std::string>::type >::value ));
    BOOST_CHECK(( boost::is_same< std::ptrdiff_t, difference_type_of<std::string>::type >::value ));

    BOOST_CHECK((  boost::is_same<                    chrono::duration<int>
                                 , difference_type_of<chrono::duration<int> >::type >::value ));
    BOOST_CHECK((  boost::is_same<                    chrono::duration<double>
                                 , difference_type_of<chrono::duration<double> >::type >::value ));

    BOOST_CHECK((  boost::is_same<                    Now::time_point::duration
                                 , difference_type_of<Now::time_point>::type >::value ));

    typedef boost::chrono::time_point<Now, boost::chrono::duration<double> > contin_timeT;
    BOOST_CHECK((  boost::is_same<                    contin_timeT::duration
                                 , difference_type_of<contin_timeT>::type >::value ));

    typedef boost::chrono::time_point<Now, boost::chrono::duration<int> > discr_timeT;
    BOOST_CHECK((  boost::is_same<                    chrono::duration<int>
                                 , difference_type_of<discr_timeT>::type >::value ));
}

BOOST_AUTO_TEST_CASE(test_size_types)
{
    BOOST_CHECK(( boost::is_same< std::size_t,    size_type_of<int>::type >::value ));
    BOOST_CHECK(( boost::is_same< std::size_t,    size_type_of<double>::type >::value ));
    BOOST_CHECK(( boost::is_same< std::size_t,    size_type_of<int*>::type >::value ));
    BOOST_CHECK(( boost::is_same< std::size_t,    size_type_of<std::string>::type >::value ));
    BOOST_CHECK(( boost::is_same<              chrono::duration<int>
                                , size_type_of<chrono::duration<int> >::type >::value ));
    BOOST_CHECK(( boost::is_same<              chrono::duration<double>
                                , size_type_of<chrono::duration<double> >::type >::value ));

    typedef boost::chrono::time_point<Now, boost::chrono::duration<int> > discr_timeT;
    BOOST_CHECK((  boost::is_same< chrono::duration<int>
                                 , size_type_of<discr_timeT>::type >::value ));

    typedef boost::chrono::time_point<Now, boost::chrono::duration<double> > contin_timeT;
    BOOST_CHECK((  boost::is_same< contin_timeT::duration
                                 , size_type_of<contin_timeT>::type >::value ));
}

BOOST_AUTO_TEST_CASE(test_chrono_identity_elements)
{
    chrono::duration<int> idel_i = icl::identity_element<chrono::duration<int> >::value();
    //cout << "dur<int>0 = " << idel_i << endl;
    chrono::duration<double> idel_d = icl::identity_element<chrono::duration<int> >::value();
    //cout << "dur<dbl>0 = " << idel_d << endl;

    BOOST_CHECK(( boost::is_same<              chrono::duration<int>
                                , size_type_of<chrono::duration<int> >::type >::value ));
}


