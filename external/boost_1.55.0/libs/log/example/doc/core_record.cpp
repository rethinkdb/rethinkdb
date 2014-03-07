/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <cstddef>
#include <iostream>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/utility/value_ref.hpp>

namespace logging = boost::log;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;
namespace keywords = boost::log::keywords;

// We define our own severity levels
enum severity_level
{
    normal,
    notification,
    warning,
    error,
    critical
};

// The operator puts a human-friendly representation of the severity level to the stream
std::ostream& operator<< (std::ostream& strm, severity_level level)
{
    static const char* strings[] =
    {
        "normal",
        "notification",
        "warning",
        "error",
        "critical"
    };

    if (static_cast< std::size_t >(level) < sizeof(strings) / sizeof(*strings))
        strm << strings[level];
    else
        strm << static_cast< int >(level);

    return strm;
}

//[ example_core_record_visitation_extraction
//=enum severity_level { ... };
//=std::ostream& operator<< (std::ostream& strm, severity_level level);

struct print_visitor
{
    typedef void result_type;
    result_type operator() (severity_level level) const
    {
        std::cout << level << std::endl;
    };
};

// Prints severity level through visitation API
void print_severity_visitation(logging::record const& rec)
{
    logging::visit< severity_level >("Severity", rec, print_visitor());
}

// Prints severity level through extraction API
void print_severity_extraction(logging::record const& rec)
{
    logging::value_ref< severity_level > level = logging::extract< severity_level >("Severity", rec);
    std::cout << level << std::endl;
}
//]

//[ example_core_record_attr_value_lookup
// Prints severity level by searching the attribute values
void print_severity_lookup(logging::record const& rec)
{
    logging::attribute_value_set const& values = rec.attribute_values();
    logging::attribute_value_set::const_iterator it = values.find("Severity");
    if (it != values.end())
    {
        logging::attribute_value const& value = it->second;

        // A single attribute value can also be visited or extracted
        std::cout << value.extract< severity_level >() << std::endl;
    }
}
//]

//[ example_core_record_subscript
BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", severity_level)

// Prints severity level by using the subscript operator
void print_severity_subscript(logging::record const& rec)
{
    // Use the attribute keyword to communicate the name and type of the value
    logging::value_ref< severity_level, tag::severity > level = rec[severity];
    std::cout << level << std::endl;
}
//]


int main(int, char*[])
{
    logging::attribute_set attrs;
    attrs.insert("Severity", attrs::make_constant(notification));

    logging::record rec = logging::core::get()->open_record(attrs);
    if (rec)
    {
        print_severity_visitation(rec);
        print_severity_extraction(rec);
        print_severity_lookup(rec);
        print_severity_subscript(rec);
    }

    return 0;
}
