//  (C) Copyright Gennadiy Rozental 2001-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision: 54633 $
//
//  Description : generators and helper macros for parameterized tests
// ***************************************************************************

#ifndef BOOST_TEST_PARAMETERIZED_TEST_HPP_021102GER
#define BOOST_TEST_PARAMETERIZED_TEST_HPP_021102GER

// Boost.Test
#include <boost/test/unit_test_suite.hpp>
#include <boost/test/utils/callback.hpp>

// Boost
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/remove_const.hpp>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

#define BOOST_PARAM_TEST_CASE( function, begin, end )                      \
boost::unit_test::make_test_case( function,                                \
                                  BOOST_TEST_STRINGIZE( function ),        \
                                  (begin), (end) )                         \
/**/

#define BOOST_PARAM_CLASS_TEST_CASE( function, tc_instance, begin, end )   \
boost::unit_test::make_test_case( function,                                \
                                  BOOST_TEST_STRINGIZE( function ),        \
                                  (tc_instance),                           \
                                  (begin), (end) )                         \
/**/

namespace boost {

namespace unit_test {

// ************************************************************************** //
// **************          test_func_with_bound_param          ************** //
// ************************************************************************** //

namespace ut_detail {

template<typename ParamType>
struct test_func_with_bound_param {
    template<typename T>
    test_func_with_bound_param( callback1<ParamType> test_func, T const& param )
    : m_test_func( test_func )
    , m_param( param )
    {}

    void operator()() { m_test_func( m_param ); }

private:
    callback1<ParamType> m_test_func;
    ParamType            m_param;
};

// ************************************************************************** //
// **************           param_test_case_generator          ************** //
// ************************************************************************** //

template<typename ParamType, typename ParamIter>
class param_test_case_generator : public test_unit_generator {
public:
    param_test_case_generator( callback1<ParamType> const&  test_func,
                               const_string                 tc_name, 
                               ParamIter                    par_begin,
                               ParamIter                    par_end )
    : m_test_func( test_func )
    , m_tc_name( ut_detail::normalize_test_case_name( tc_name ) )
    , m_par_begin( par_begin )
    , m_par_end( par_end )
    {}

    virtual test_unit* next() const
    {
        if( m_par_begin == m_par_end )
            return (test_unit*)0;

        test_func_with_bound_param<ParamType> bound_test_func( m_test_func, *m_par_begin );
        test_unit* res = new test_case( m_tc_name, bound_test_func );

        ++m_par_begin;

        return res;
    }

private:
    // Data members
    callback1<ParamType>    m_test_func;
    std::string             m_tc_name;
    mutable ParamIter       m_par_begin;
    ParamIter               m_par_end;
};

//____________________________________________________________________________//

template<typename UserTestCase,typename ParamType>
struct user_param_tc_method_invoker {
    typedef void (UserTestCase::*test_method)( ParamType );

    // Constructor
    user_param_tc_method_invoker( shared_ptr<UserTestCase> inst, test_method test_method )
    : m_inst( inst ), m_test_method( test_method ) {}

    void operator()( ParamType p ) { ((*m_inst).*m_test_method)( p ); }

    // Data members
    shared_ptr<UserTestCase> m_inst;
    test_method              m_test_method;
};

//____________________________________________________________________________//

} // namespace ut_detail

template<typename ParamType, typename ParamIter>
inline ut_detail::param_test_case_generator<ParamType,ParamIter>
make_test_case( callback1<ParamType> const& test_func,
                const_string   tc_name, 
                ParamIter      par_begin,
                ParamIter      par_end )
{
    return ut_detail::param_test_case_generator<ParamType,ParamIter>( test_func, tc_name, par_begin, par_end );
}

//____________________________________________________________________________//

template<typename ParamType, typename ParamIter>
inline ut_detail::param_test_case_generator<
    BOOST_DEDUCED_TYPENAME remove_const<BOOST_DEDUCED_TYPENAME remove_reference<ParamType>::type>::type,ParamIter>
make_test_case( void (*test_func)( ParamType ),
                const_string  tc_name, 
                ParamIter     par_begin,
                ParamIter     par_end )
{
    typedef BOOST_DEDUCED_TYPENAME remove_const<BOOST_DEDUCED_TYPENAME remove_reference<ParamType>::type>::type param_value_type;
    return ut_detail::param_test_case_generator<param_value_type,ParamIter>( test_func, tc_name, par_begin, par_end );
}

//____________________________________________________________________________//

template<typename UserTestCase,typename ParamType, typename ParamIter>
inline ut_detail::param_test_case_generator<
    BOOST_DEDUCED_TYPENAME remove_const<BOOST_DEDUCED_TYPENAME remove_reference<ParamType>::type>::type,ParamIter>
make_test_case( void (UserTestCase::*test_method )( ParamType ),
                const_string                           tc_name,
                boost::shared_ptr<UserTestCase> const& user_test_case,
                ParamIter                              par_begin,
                ParamIter                              par_end )
{
    typedef BOOST_DEDUCED_TYPENAME remove_const<BOOST_DEDUCED_TYPENAME remove_reference<ParamType>::type>::type param_value_type;
    return ut_detail::param_test_case_generator<param_value_type,ParamIter>( 
               ut_detail::user_param_tc_method_invoker<UserTestCase,ParamType>( user_test_case, test_method ), 
               tc_name,
               par_begin,
               par_end );
}

//____________________________________________________________________________//

} // unit_test

} // namespace boost

//____________________________________________________________________________//

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_PARAMETERIZED_TEST_HPP_021102GER

