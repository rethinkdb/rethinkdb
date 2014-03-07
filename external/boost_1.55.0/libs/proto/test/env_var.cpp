///////////////////////////////////////////////////////////////////////////////
// env_var.cpp
//
//  Copyright 2012 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <cstring>
#include <sstream>
#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/proto/proto.hpp>
#include <boost/test/unit_test.hpp>

namespace proto = boost::proto;

BOOST_PROTO_DEFINE_ENV_VAR(tag0_type, tag0);

struct abstract
{
    virtual ~abstract() = 0;
};

abstract::~abstract() {}

struct concrete : abstract
{
    ~concrete() {}
};

template<typename Tag, typename Env>
void assert_has_env_var(Env const &)
{
    BOOST_MPL_ASSERT((proto::result_of::has_env_var<Env, Tag>));
}

template<typename Tag, typename Env>
void assert_has_env_var_not(Env const &)
{
    BOOST_MPL_ASSERT_NOT((proto::result_of::has_env_var<Env, Tag>));
}

void test_is_env()
{
    BOOST_MPL_ASSERT_NOT((proto::is_env<int>));
    BOOST_MPL_ASSERT_NOT((proto::is_env<int &>));
    BOOST_MPL_ASSERT_NOT((proto::is_env<int const &>));

    BOOST_MPL_ASSERT_NOT((proto::is_env<abstract>));
    BOOST_MPL_ASSERT_NOT((proto::is_env<abstract &>));
    BOOST_MPL_ASSERT_NOT((proto::is_env<abstract const &>));

    BOOST_MPL_ASSERT((proto::is_env<proto::empty_env>));
    BOOST_MPL_ASSERT((proto::is_env<proto::empty_env &>));
    BOOST_MPL_ASSERT((proto::is_env<proto::empty_env const &>));

    BOOST_MPL_ASSERT((proto::is_env<proto::env<tag0_type, int> >));
    BOOST_MPL_ASSERT((proto::is_env<proto::env<tag0_type, int> &>));
    BOOST_MPL_ASSERT((proto::is_env<proto::env<tag0_type, int> const &>));
}

void test_as_env()
{
    proto::env<proto::data_type, int> e0 = proto::as_env(2);
    BOOST_CHECK_EQUAL(e0[proto::data], 2);
    assert_has_env_var<proto::data_type>(e0);
    assert_has_env_var_not<tag0_type>(e0);

    int i = 39;
    proto::env<proto::data_type, int &> e1 = proto::as_env(boost::ref(i));
    assert_has_env_var<proto::data_type>(i);
    assert_has_env_var_not<tag0_type>(i);
    BOOST_CHECK_EQUAL(e1[proto::data], 39);
    BOOST_CHECK_EQUAL(&e1[proto::data], &i);

    proto::empty_env e2 = proto::as_env(proto::empty_env());
    proto::env<proto::data_type, int &> e3 = proto::as_env(e1);
    proto::env<proto::data_type, int &> & e4 = proto::as_env(boost::ref(e1));
    BOOST_CHECK_EQUAL(&e4, &e1);

    concrete c;
    abstract &a = c;
    std::stringstream sout;
    int rgi[2] = {};
    proto::env<proto::data_type, abstract &> e5 = proto::as_env(a);
    proto::env<proto::data_type, std::stringstream &> e6 = proto::as_env(sout);
    BOOST_CHECK_EQUAL(&e6[proto::data], &sout);
    proto::env<proto::data_type, int(&)[2]> e7 = proto::as_env(rgi);
    BOOST_CHECK_EQUAL(&e7[proto::data][0], &rgi[0]);
    proto::env<proto::data_type, void(&)()> e8 = proto::as_env(test_as_env);
    BOOST_CHECK_EQUAL(&e8[proto::data], &test_as_env);
}

void test_comma()
{
    proto::env<proto::data_type, int> e0 = (proto::data = 1);
    BOOST_CHECK_EQUAL(e0[proto::data], 1);

    int i = 39;
    proto::env<proto::data_type, int &> e1 = (proto::data = boost::ref(i));
    BOOST_CHECK_EQUAL(e1[proto::data], 39);
    BOOST_CHECK_EQUAL(&e1[proto::data], &i);

    concrete c;
    abstract &a = c;
    std::stringstream sout;
    int rgi[2] = {};
    proto::env<proto::data_type, abstract &> e5 = (proto::data = a);
    proto::env<proto::data_type, std::stringstream &> e6 = (proto::data = sout);
    BOOST_CHECK_EQUAL(&e6[proto::data], &sout);
    proto::env<proto::data_type, int(&)[2]> e7 = (proto::data = rgi);
    BOOST_CHECK_EQUAL(&e7[proto::data][0], &rgi[0]);
    // The test below fails on msvc due to a compiler bug
    // note: <https://connect.microsoft.com/VisualStudio/feedback/details/754684/premature-decay-of-function-types-in-overloaded-assignment-operator>
#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1600))
    proto::env<proto::data_type, void(&)()> e8 = (proto::data = boost::ref(test_as_env));
    BOOST_CHECK_EQUAL(&e8[proto::data], &test_as_env);
#else
    proto::env<proto::data_type, void(&)()> e8 = (proto::data = test_as_env);
    BOOST_CHECK_EQUAL(&e8[proto::data], &test_as_env);
#endif

    proto::env<
        tag0_type
      , char const (&)[6]
      , proto::env<proto::data_type, int>
    > e9 = (proto::data = 1, tag0 = "hello");
    BOOST_CHECK_EQUAL(e9[proto::data], 1);
    BOOST_CHECK_EQUAL(0, std::strcmp(e9[tag0], "hello"));

    proto::env<
        tag0_type
      , int
      , proto::env<
            tag0_type
          , char const (&)[6]
          , proto::env<proto::data_type, int>
        >
    > e10 = (proto::data = 1, tag0 = "hello", tag0 = 42);
    BOOST_CHECK_EQUAL(e10[proto::data], 1);
    BOOST_CHECK_EQUAL(e10[tag0], 42);

    proto::env<
        tag0_type
      , char const (&)[6]
      , proto::env<proto::data_type, abstract &>
    > e11 = (a, tag0 = "hello");
    BOOST_CHECK_EQUAL(&e11[proto::data], &a);
    BOOST_CHECK_EQUAL(0, std::strcmp(e11[tag0], "hello"));

    proto::env<
        tag0_type
      , int
      , proto::env<
            tag0_type
          , char const (&)[6]
          , proto::env<proto::data_type, abstract &>
        >
    > e12 = (a, tag0 = "hello", tag0 = 42);
    BOOST_CHECK_EQUAL(&e12[proto::data], &a);
    BOOST_CHECK_EQUAL(e12[tag0], 42);

    proto::env<tag0_type, int> e13 = (proto::empty_env(), tag0 = 42);
    BOOST_CHECK_EQUAL(e13[tag0], 42);
    assert_has_env_var<tag0_type>(e13);
    assert_has_env_var_not<proto::data_type>(e13);

    proto::empty_env empty;
    proto::env<tag0_type, int> e14 = (boost::ref(empty), tag0 = 42);
    BOOST_CHECK_EQUAL(e14[tag0], 42);

    proto::env<
        proto::data_type
      , char const (&)[6]
      , proto::env<tag0_type, int>
    > e15 = (boost::ref(e14), proto::data = "hello");
    BOOST_CHECK_EQUAL(e15[tag0], 42);
    BOOST_CHECK_EQUAL(0, std::strcmp(e15[proto::data], "hello"));

    proto::env<
        proto::data_type
      , char const (&)[6]
      , proto::env<tag0_type, int>
    > e16 = (proto::as_env(boost::ref(e14)), proto::data = "hello");
    BOOST_CHECK_EQUAL(e16[tag0], 42);
    BOOST_CHECK_EQUAL(0, std::strcmp(e16[proto::data], "hello"));
}

void test_result_of_env_var()
{
    typedef proto::empty_env env0_type;
    BOOST_MPL_ASSERT((boost::is_same<proto::result_of::env_var<env0_type, proto::data_type>::type, proto::key_not_found>));
    BOOST_MPL_ASSERT((boost::is_same<proto::result_of::env_var<env0_type &, proto::data_type>::type, proto::key_not_found>));
    BOOST_MPL_ASSERT((boost::is_same<proto::result_of::env_var<env0_type const &, proto::data_type>::type, proto::key_not_found>));

    typedef proto::env<proto::data_type, int> env1_type;
    BOOST_MPL_ASSERT((boost::is_same<proto::result_of::env_var<env1_type, proto::data_type>::type, int>));
    BOOST_MPL_ASSERT((boost::is_same<proto::result_of::env_var<env1_type &, proto::data_type>::type, int>));
    BOOST_MPL_ASSERT((boost::is_same<proto::result_of::env_var<env1_type const &, proto::data_type>::type, int>));

    typedef proto::env<proto::data_type, int &> env2_type;
    BOOST_MPL_ASSERT((boost::is_same<proto::result_of::env_var<env2_type, proto::data_type>::type, int &>));
    BOOST_MPL_ASSERT((boost::is_same<proto::result_of::env_var<env2_type &, proto::data_type>::type, int &>));
    BOOST_MPL_ASSERT((boost::is_same<proto::result_of::env_var<env2_type const &, proto::data_type>::type, int &>));

    typedef proto::env<proto::data_type, double, proto::env<tag0_type, abstract &> > env3_type;
    BOOST_MPL_ASSERT((boost::is_same<proto::result_of::env_var<env3_type, proto::data_type>::type, double>));
    BOOST_MPL_ASSERT((boost::is_same<proto::result_of::env_var<env3_type, tag0_type>::type, abstract &>));
    BOOST_MPL_ASSERT((boost::is_same<proto::result_of::env_var<env3_type &, proto::data_type>::type, double>));
    BOOST_MPL_ASSERT((boost::is_same<proto::result_of::env_var<env3_type &, tag0_type>::type, abstract &>));
    BOOST_MPL_ASSERT((boost::is_same<proto::result_of::env_var<env3_type const &, proto::data_type>::type, double>));
    BOOST_MPL_ASSERT((boost::is_same<proto::result_of::env_var<env3_type const &, tag0_type>::type, abstract &>));

    typedef proto::env<tag0_type, double, proto::env<tag0_type, abstract &> > env4_type;
    BOOST_MPL_ASSERT((boost::is_same<proto::result_of::env_var<env4_type, tag0_type>::type, double>));
    BOOST_MPL_ASSERT((boost::is_same<proto::result_of::env_var<env4_type &, tag0_type>::type, double>));
    BOOST_MPL_ASSERT((boost::is_same<proto::result_of::env_var<env4_type const &, tag0_type>::type, double>));
}

void test_env_var()
{
    proto::key_not_found x0 = proto::env_var<proto::data_type>(proto::empty_env());
    proto::key_not_found x1 = proto::env_var<proto::data_type>(tag0 = 42);
    int x2 = proto::env_var<tag0_type>(tag0 = 42);
    BOOST_CHECK_EQUAL(x2, 42);
    int x3 = proto::functional::env_var<tag0_type>()(tag0 = 42);
    BOOST_CHECK_EQUAL(x3, 42);

    int i = 43;
    int & x4 = proto::env_var<tag0_type>(tag0 = boost::ref(i));
    BOOST_CHECK_EQUAL(&x4, &i);
    int & x5 = proto::functional::env_var<tag0_type>()(tag0 = boost::ref(i));
    BOOST_CHECK_EQUAL(&x5, &i);

    concrete c;
    abstract &a = c;
    abstract &x6 = proto::env_var<tag0_type>(tag0 = a);
    BOOST_CHECK_EQUAL(&x6, &a);
    abstract &x7 = proto::functional::env_var<tag0_type>()(tag0 = a);
    BOOST_CHECK_EQUAL(&x7, &a);

    abstract &x8 = proto::env_var<tag0_type>((42, tag0 = a));
    BOOST_CHECK_EQUAL(&x8, &a);
    abstract &x9 = proto::functional::env_var<tag0_type>()((42, tag0 = a));
    BOOST_CHECK_EQUAL(&x9, &a);
}

void test_env_var_tfx()
{
    typedef proto::terminal<int>::type int_;
    int_ i = {42};

    // tests for _env
    BOOST_MPL_ASSERT((boost::is_same<boost::result_of<proto::_env(int_ &)>::type, proto::empty_env>));
    BOOST_MPL_ASSERT((boost::is_same<boost::result_of<proto::_env(int_ &, int)>::type, proto::empty_env>));
    BOOST_MPL_ASSERT((boost::is_same<boost::result_of<proto::_env(int_ &, int &, float &)>::type, float &>));

    // Bummer, is there any way around this?
#ifdef BOOST_RESULT_OF_USE_DECLTYPE
    BOOST_MPL_ASSERT((boost::is_same<boost::result_of<proto::_env(int_ &, int &, float)>::type, float const &>));
    BOOST_MPL_ASSERT((boost::is_same<boost::tr1_result_of<proto::_env(int_ &, int &, float)>::type, float>));
#else
    BOOST_MPL_ASSERT((boost::is_same<boost::result_of<proto::_env(int_ &, int &, float)>::type, float>));
    BOOST_MPL_ASSERT((boost::is_same<boost::tr1_result_of<proto::_env(int_ &, int &, float)>::type, float>));
#endif

    double d = 3.14;
    double & rd = proto::_env()(i, 0, d);
    BOOST_CHECK_EQUAL(&d, &rd);

    proto::env<proto::data_type, int> e0 = proto::_env()(i, 0, proto::as_env(42));
    BOOST_CHECK_EQUAL(e0[proto::data], 42);

    proto::env<proto::data_type, int> e1 = proto::_env()(i, 0, proto::functional::as_env()(42));
    BOOST_CHECK_EQUAL(e1[proto::data], 42);

    proto::env<proto::data_type, int> e2 = proto::_env()(i, 0, (proto::data = 42));
    BOOST_CHECK_EQUAL(e2[proto::data], 42);

    proto::env<proto::data_type, int, proto::env<proto::data_type, int> > e3 = proto::_env()(i, 0, (42, proto::data = 43));
    BOOST_CHECK_EQUAL(e3[proto::data], 43);
}

using namespace boost::unit_test;
///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("test for environment variables");
    test->add(BOOST_TEST_CASE(&test_as_env));
    test->add(BOOST_TEST_CASE(&test_comma));
    test->add(BOOST_TEST_CASE(&test_result_of_env_var));
    test->add(BOOST_TEST_CASE(&test_env_var));
    test->add(BOOST_TEST_CASE(&test_env_var_tfx));
    return test;
}
