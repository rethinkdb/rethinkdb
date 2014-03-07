//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c) 2011 Colin Rundel
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/mpl/print.hpp>
#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/fusion/include/adapt_adt.hpp>

#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/support_adapt_adt_attributes.hpp>

#include "test.hpp"

///////////////////////////////////////////////////////////////////////////////
class data1
{
private:
    int width_;
    int height_;

public:
    data1()
      : width_(400), height_(400) 
    {}

    data1(int width, int height)
      : width_(width), height_(height) 
    {}

    int const& width() const { return width_;}
    int const& height() const { return height_;}

    void set_width(int width) { width_ = width;}
    void set_height(int height) { height_ = height;}
};

BOOST_FUSION_ADAPT_ADT(
    data1,
    (int, int const&, obj.width(),  obj.set_width(val))
    (int, int const&, obj.height(), obj.set_height(val))
);

///////////////////////////////////////////////////////////////////////////////
class data2
{
private:
    std::string data_;

public:
    data2()
      : data_("test")
    {}

    data2(std::string const& data)
      : data_(data)
    {}

    std::string const& data() const { return data_;}
    void set_data(std::string const& data) { data_ = data;}
};

BOOST_FUSION_ADAPT_ADT(
    data2, 
    (std::string, std::string const&, obj.data(), obj.set_data(val))
);

///////////////////////////////////////////////////////////////////////////////
class data3
{
private:
    double data_;

public:
    data3(double data = 0.0)
      : data_(data) 
    {}

    double const& data() const { return data_;}
    void set_data(double data) { data_ = data;}
};

BOOST_FUSION_ADAPT_ADT(
    data3,
    (double, double const&, obj.data(), obj.set_data(val))
);

///////////////////////////////////////////////////////////////////////////////
class data4 
{
public:
    boost::optional<int> a_;
    boost::optional<double> b_;
    boost::optional<std::string> c_;

    boost::optional<int> const& a() const { return a_; }
    boost::optional<double> const& b() const { return b_; }
    boost::optional<std::string> const& c() const { return c_; }
};


BOOST_FUSION_ADAPT_ADT(
    data4,
    (boost::optional<int>, boost::optional<int> const&, obj.a(), /**/)
    (boost::optional<double>, boost::optional<double> const&, obj.b(), /**/)
    (boost::optional<std::string>, boost::optional<std::string> const&, obj.c(), /**/)
);

///////////////////////////////////////////////////////////////////////////////
int main () 
{
    using spirit_test::test;

    {
        using boost::spirit::karma::int_;

        data1 b(800, 600);
        BOOST_TEST(test("width: 800\nheight: 600\n", 
            "width: " << int_ << "\n" << "height: " << int_ << "\n", b));
    }

    {
        using boost::spirit::karma::char_;
        using boost::spirit::karma::string;

        data2 d("test");
        BOOST_TEST(test("data: test\n", "data: " << +char_ << "\n", d));
        BOOST_TEST(test("data: test\n", "data: " << string << "\n", d));
    }

    {
        using boost::spirit::karma::double_;

        BOOST_TEST(test("x=0.0\n", "x=" << double_ << "\n", data3(0)));
        BOOST_TEST(test("x=1.1\n", "x=" << double_ << "\n", data3(1.1)));
        BOOST_TEST(test("x=1.0e10\n", "x=" << double_ << "\n", data3(1e10)));

        BOOST_TEST(test("x=inf\n", "x=" << double_ << "\n", 
            data3(std::numeric_limits<double>::infinity())));
        if (std::numeric_limits<double>::has_quiet_NaN) {
            BOOST_TEST(test("x=nan\n", "x=" << double_ << "\n", 
                data3(std::numeric_limits<double>::quiet_NaN())));
        }
        if (std::numeric_limits<double>::has_signaling_NaN) {
            BOOST_TEST(test("x=nan\n", "x=" << double_ << "\n", 
                data3(std::numeric_limits<double>::signaling_NaN())));
        }
    }

    {
        using boost::spirit::karma::double_;
        using boost::spirit::karma::int_;
        using boost::spirit::karma::string;

        data4 d;
        d.b_ = 10;

        BOOST_TEST(test(
            "Testing: b: 10.0\n", 
            "Testing: " << -("a: " << int_ << "\n")
                        << -("b: " << double_ << "\n")
                        << -("c: " << string << "\n"), d));
    }

    return boost::report_errors();
}
