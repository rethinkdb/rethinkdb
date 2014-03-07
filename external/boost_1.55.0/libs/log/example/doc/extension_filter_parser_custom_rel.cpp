/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <string>
#include <iostream>
#include <stdexcept>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/phoenix.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes/attribute_name.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/value_ref.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/utility/setup/filter_parser.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>

namespace logging = boost::log;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;

struct point
{
    float m_x, m_y;

    point() : m_x(0.0f), m_y(0.0f) {}
    point(float x, float y) : m_x(x), m_y(y) {}
};

bool operator== (point const& left, point const& right);
bool operator!= (point const& left, point const& right);

template< typename CharT, typename TraitsT >
std::basic_ostream< CharT, TraitsT >& operator<< (std::basic_ostream< CharT, TraitsT >& strm, point const& p);
template< typename CharT, typename TraitsT >
std::basic_istream< CharT, TraitsT >& operator>> (std::basic_istream< CharT, TraitsT >& strm, point& p);

const float epsilon = 0.0001f;

bool operator== (point const& left, point const& right)
{
    return (left.m_x - epsilon <= right.m_x && left.m_x + epsilon >= right.m_x) &&
           (left.m_y - epsilon <= right.m_y && left.m_y + epsilon >= right.m_y);
}

bool operator!= (point const& left, point const& right)
{
    return !(left == right);
}

template< typename CharT, typename TraitsT >
std::basic_ostream< CharT, TraitsT >& operator<< (std::basic_ostream< CharT, TraitsT >& strm, point const& p)
{
    if (strm.good())
        strm << "(" << p.m_x << ", " << p.m_y << ")";
    return strm;
}

template< typename CharT, typename TraitsT >
std::basic_istream< CharT, TraitsT >& operator>> (std::basic_istream< CharT, TraitsT >& strm, point& p)
{
    if (strm.good())
    {
        CharT left_brace = static_cast< CharT >(0), comma = static_cast< CharT >(0), right_brace = static_cast< CharT >(0);
        strm.setf(std::ios_base::skipws);
        strm >> left_brace >> p.m_x >> comma >> p.m_y >> right_brace;
        if (left_brace != '(' || comma != ',' || right_brace != ')')
            strm.setstate(std::ios_base::failbit);
    }
    return strm;
}

//[ example_extension_filter_parser_rectangle_definition
struct rectangle
{
    point m_top_left, m_bottom_right;
};

template< typename CharT, typename TraitsT >
std::basic_ostream< CharT, TraitsT >& operator<< (std::basic_ostream< CharT, TraitsT >& strm, rectangle const& r);
template< typename CharT, typename TraitsT >
std::basic_istream< CharT, TraitsT >& operator>> (std::basic_istream< CharT, TraitsT >& strm, rectangle& r);
//]

template< typename CharT, typename TraitsT >
std::basic_ostream< CharT, TraitsT >& operator<< (std::basic_ostream< CharT, TraitsT >& strm, rectangle const& r)
{
    if (strm.good())
        strm << "{" << r.m_top_left << " - " << r.m_bottom_right << "}";
    return strm;
}

template< typename CharT, typename TraitsT >
std::basic_istream< CharT, TraitsT >& operator>> (std::basic_istream< CharT, TraitsT >& strm, rectangle& r)
{
    if (strm.good())
    {
        CharT left_brace = static_cast< CharT >(0), dash = static_cast< CharT >(0), right_brace = static_cast< CharT >(0);
        strm.setf(std::ios_base::skipws);
        strm >> left_brace >> r.m_top_left >> dash >> r.m_bottom_right >> right_brace;
        if (left_brace != '{' || dash != '-' || right_brace != '}')
            strm.setstate(std::ios_base::failbit);
    }
    return strm;
}


//[ example_extension_custom_filter_factory_with_custom_rel
// The function checks if the point is inside the rectangle
bool is_in_rect(logging::value_ref< point > const& p, rectangle const& r)
{
    if (p)
    {
        return p->m_x >= r.m_top_left.m_x && p->m_x <= r.m_bottom_right.m_x &&
               p->m_y >= r.m_top_left.m_y && p->m_y <= r.m_bottom_right.m_y;
    }
    return false;
}

// Custom point filter factory
class point_filter_factory :
    public logging::filter_factory< char >
{
public:
    logging::filter on_exists_test(logging::attribute_name const& name)
    {
        return expr::has_attr< point >(name);
    }

    logging::filter on_equality_relation(logging::attribute_name const& name, string_type const& arg)
    {
        return expr::attr< point >(name) == boost::lexical_cast< point >(arg);
    }

    logging::filter on_inequality_relation(logging::attribute_name const& name, string_type const& arg)
    {
        return expr::attr< point >(name) != boost::lexical_cast< point >(arg);
    }

    logging::filter on_custom_relation(logging::attribute_name const& name, string_type const& rel, string_type const& arg)
    {
        if (rel == "is_in_rect")
        {
            return boost::phoenix::bind(&is_in_rect, expr::attr< point >(name), boost::lexical_cast< rectangle >(arg));
        }
        throw std::runtime_error("Unsupported filter relation: " + rel);
    }
};

void init_factories()
{
    //<-
    logging::register_simple_formatter_factory< point, char >("Coordinates");
    //->
    logging::register_filter_factory("Coordinates", boost::make_shared< point_filter_factory >());
}
//]

void init_logging()
{
    init_factories();

    logging::add_console_log
    (
        std::clog,
        keywords::filter = "%Coordinates% is_in_rect \"{(0, 0) - (20, 20)}\"",
        keywords::format = "%TimeStamp% %Coordinates% %Message%"
    );

    logging::add_common_attributes();
}

int main(int, char*[])
{
    init_logging();

    src::logger lg;

    // We have to use scoped attributes in order coordinates to be passed to filters
    {
        BOOST_LOG_SCOPED_LOGGER_TAG(lg, "Coordinates", point(10, 10));
        BOOST_LOG(lg) << "Hello, world with coordinates (10, 10)!";
    }
    {
        BOOST_LOG_SCOPED_LOGGER_TAG(lg, "Coordinates", point(50, 50));
        BOOST_LOG(lg) << "Hello, world with coordinates (50, 50)!"; // this message will be suppressed by filter
    }

    return 0;
}
