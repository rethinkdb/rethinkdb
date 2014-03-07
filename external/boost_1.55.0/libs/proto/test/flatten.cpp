///////////////////////////////////////////////////////////////////////////////
// proto_fusion_s.cpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/proto/core.hpp>
#include <boost/proto/fusion.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/utility/addressof.hpp>
#include <sstream>

std::ostream &operator <<(std::ostream &sout, boost::proto::tag::shift_right)
{
    return sout << ">>";
}

std::ostream &operator <<(std::ostream &sout, boost::proto::tag::bitwise_or)
{
    return sout << "|";
}

template<typename Args>
std::ostream &operator <<(std::ostream &sout, boost::proto::expr<boost::proto::tag::terminal, Args, 0> const *op)
{
    return sout << boost::proto::value(*op);
}

template<typename Args>
std::ostream &operator <<(std::ostream &sout, boost::proto::basic_expr<boost::proto::tag::terminal, Args, 0> const *op)
{
    return sout << boost::proto::value(*op);
}

template<typename Tag, typename Args>
std::ostream &operator <<(std::ostream &sout, boost::proto::expr<Tag, Args, 1> const *op)
{
    return sout << Tag() << boost::addressof(boost::proto::child(*op).proto_base());
}

template<typename Tag, typename Args>
std::ostream &operator <<(std::ostream &sout, boost::proto::basic_expr<Tag, Args, 1> const *op)
{
    return sout << Tag() << boost::addressof(boost::proto::child(*op).proto_base());
}

template<typename Tag, typename Args>
std::ostream &operator <<(std::ostream &sout, boost::proto::expr<Tag, Args, 2> const *op)
{
    return sout << boost::addressof(boost::proto::left(*op).proto_base()) << Tag() << boost::addressof(boost::proto::right(*op).proto_base());
}

template<typename Tag, typename Args>
std::ostream &operator <<(std::ostream &sout, boost::proto::basic_expr<Tag, Args, 2> const *op)
{
    return sout << boost::addressof(boost::proto::left(*op).proto_base()) << Tag() << boost::addressof(boost::proto::right(*op).proto_base());
}

///////////////////////////////////////////////////////////////////////////////
// to_string
//
struct to_string
{
    to_string(std::ostream &sout)
      : sout_(sout)
    {}

    template<typename Op>
    void operator ()(Op const &op) const
    {
        this->sout_ << '(' << boost::addressof(op.proto_base()) << ')';
    }
private:
    std::ostream &sout_;
};

void test1()
{
    using boost::proto::flatten;

    boost::proto::terminal<char>::type a_ = {'a'};
    boost::proto::terminal<char>::type b_ = {'b'};
    boost::proto::terminal<char>::type c_ = {'c'};
    boost::proto::terminal<char>::type d_ = {'d'};
    boost::proto::terminal<char>::type e_ = {'e'};
    boost::proto::terminal<char>::type f_ = {'f'};
    boost::proto::terminal<char>::type g_ = {'g'};
    boost::proto::terminal<char>::type h_ = {'h'};
    boost::proto::terminal<char>::type i_ = {'i'};

    std::stringstream sout;

    // Test for 1-way branching "tree"
    sout.str("");
    boost::fusion::for_each(flatten(!!!!(a_ >> b_)), to_string(sout));
    BOOST_CHECK_EQUAL("(a>>b)", sout.str());

    // Tests for 2-way branching trees
    sout.str("");
    boost::fusion::for_each(flatten(a_ >> b_ >> c_), to_string(sout));
    BOOST_CHECK_EQUAL("(a)(b)(c)", sout.str());

    sout.str("");
    boost::fusion::for_each(flatten(a_ | b_ | c_), to_string(sout));
    BOOST_CHECK_EQUAL("(a)(b)(c)", sout.str());

    sout.str("");
    boost::fusion::for_each(flatten(a_ >> b_ | c_ >> d_), to_string(sout));
    BOOST_CHECK_EQUAL("(a>>b)(c>>d)", sout.str());

    sout.str("");
    boost::fusion::for_each(flatten(a_ | b_ >> c_ | d_), to_string(sout));
    BOOST_CHECK_EQUAL("(a)(b>>c)(d)", sout.str());

    sout.str("");
    boost::fusion::for_each(flatten(a_ >> b_ | c_ >> d_ | e_ >> f_ >> g_), to_string(sout));
    BOOST_CHECK_EQUAL("(a>>b)(c>>d)(e>>f>>g)", sout.str());

    sout.str("");
    boost::fusion::for_each(flatten(a_ >> b_ | c_ >> d_ | e_ >> (f_ | g_) >> h_), to_string(sout));
    BOOST_CHECK_EQUAL("(a>>b)(c>>d)(e>>f|g>>h)", sout.str());

    // Test for n-way branching tree
    sout.str("");
    boost::fusion::for_each(flatten(a_(b_(c_ >> d_, e_ | f_), g_ >> h_)(i_)), to_string(sout));
    BOOST_CHECK_EQUAL("(a)(b)(c>>d)(e|f)(g>>h)(i)", sout.str());
}

////////////////////////////////////////////////////////////////////////
// Test that EXTENDS expression wrappers are also valid fusion sequences

template<typename Expr>
struct My;

struct MyDomain
  : boost::proto::domain<boost::proto::pod_generator<My> >
{};

template<typename Expr>
struct My
{
    BOOST_PROTO_EXTENDS(Expr, My<Expr>, MyDomain)
};

void test2()
{
    using boost::proto::flatten;

    My<boost::proto::terminal<char>::type> a_ = {{'a'}};
    My<boost::proto::terminal<char>::type> b_ = {{'b'}};
    My<boost::proto::terminal<char>::type> c_ = {{'c'}};
    My<boost::proto::terminal<char>::type> d_ = {{'d'}};
    My<boost::proto::terminal<char>::type> e_ = {{'e'}};
    My<boost::proto::terminal<char>::type> f_ = {{'f'}};
    My<boost::proto::terminal<char>::type> g_ = {{'g'}};
    My<boost::proto::terminal<char>::type> h_ = {{'h'}};
    My<boost::proto::terminal<char>::type> i_ = {{'i'}};

    std::stringstream sout;

    // Test for 1-way branching "tree"
    sout.str("");
    boost::fusion::for_each(flatten(!!!!(a_ >> b_)), to_string(sout));
    BOOST_CHECK_EQUAL("(a>>b)", sout.str());

    // Tests for 2-way branching trees
    sout.str("");
    boost::fusion::for_each(flatten(a_ >> b_ >> c_), to_string(sout));
    BOOST_CHECK_EQUAL("(a)(b)(c)", sout.str());

    sout.str("");
    boost::fusion::for_each(flatten(a_ | b_ | c_), to_string(sout));
    BOOST_CHECK_EQUAL("(a)(b)(c)", sout.str());

    sout.str("");
    boost::fusion::for_each(flatten(a_ >> b_ | c_ >> d_), to_string(sout));
    BOOST_CHECK_EQUAL("(a>>b)(c>>d)", sout.str());

    sout.str("");
    boost::fusion::for_each(flatten(a_ | b_ >> c_ | d_), to_string(sout));
    BOOST_CHECK_EQUAL("(a)(b>>c)(d)", sout.str());

    sout.str("");
    boost::fusion::for_each(flatten(a_ >> b_ | c_ >> d_ | e_ >> f_ >> g_), to_string(sout));
    BOOST_CHECK_EQUAL("(a>>b)(c>>d)(e>>f>>g)", sout.str());

    sout.str("");
    boost::fusion::for_each(flatten(a_ >> b_ | c_ >> d_ | e_ >> (f_ | g_) >> h_), to_string(sout));
    BOOST_CHECK_EQUAL("(a>>b)(c>>d)(e>>f|g>>h)", sout.str());

    // Test for n-way branching tree
    sout.str("");
    boost::fusion::for_each(flatten(a_(b_(c_ >> d_, e_ | f_), g_ >> h_)(i_)), to_string(sout));
    BOOST_CHECK_EQUAL("(a)(b)(c>>d)(e|f)(g>>h)(i)", sout.str());
}

using namespace boost::unit_test;
///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("test proto and segmented fusion integration");

    test->add(BOOST_TEST_CASE(&test1));
    test->add(BOOST_TEST_CASE(&test2));

    return test;
}
