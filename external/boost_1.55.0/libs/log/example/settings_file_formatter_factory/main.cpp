/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   main.cpp
 * \author Andrey Semashev
 * \date   12.05.2010
 *
 * \brief  An example of initializing the library from a settings file,
 *         with a custom formatter for an attribute.
 */

// #define BOOST_ALL_DYN_LINK 1

#include <string>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/log/core.hpp>
#include <boost/log/common.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/core/record.hpp>
#include <boost/log/attributes/value_visitation.hpp>
#include <boost/log/utility/setup/from_stream.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/formatter_parser.hpp>

namespace logging = boost::log;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;

enum severity_level
{
    normal,
    notification,
    warning,
    error,
    critical
};

BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(test_lg, src::severity_logger< >)

//! Our custom formatter for the scope list
struct scope_list_formatter
{
    typedef void result_type;
    typedef attrs::named_scope::value_type scope_stack;

    explicit scope_list_formatter(logging::attribute_name const& name) :
        name_(name)
    {
    }
    void operator()(logging::record_view const& rec, logging::formatting_ostream& strm) const
    {
        // We need to acquire the attribute value from the log record
        logging::visit< scope_stack >
        (
            name_,
            rec.attribute_values(),
            boost::bind(&scope_list_formatter::format, _1, boost::ref(strm))
        );
    }

private:
    //! This is where our custom formatting takes place
    static void format(scope_stack const& scopes, logging::formatting_ostream& strm)
    {
        scope_stack::const_iterator it = scopes.begin(), end = scopes.end();
        for (; it != end; ++it)
        {
            strm << "\t" << it->scope_name << " [" << it->file_name << ":" << it->line << "]\n";
        }
    }

private:
    logging::attribute_name name_;
};

class my_scopes_formatter_factory :
    public logging::formatter_factory< char >
{
public:
    /*!
     * This function creates a formatter for the MyScopes attribute.
     * It effectively associates the attribute with the scope_list_formatter class
     */
    formatter_type create_formatter(
        logging::attribute_name const& attr_name, args_map const& args)
    {
        return formatter_type(scope_list_formatter(attr_name));
    }
};

//! The function initializes the logging library
void init_logging()
{
    // First thing - register the custom formatter for MyScopes
    logging::register_formatter_factory("MyScopes", boost::make_shared< my_scopes_formatter_factory >());

    // Then load the settings from the file
    std::ifstream settings("settings.txt");
    if (!settings.is_open())
        throw std::runtime_error("Could not open settings.txt file");
    logging::init_from_stream(settings);

    // Add some attributes
    logging::add_common_attributes();

    logging::core::get()->add_global_attribute("MyScopes", attrs::named_scope());
}

//! The function tests logging
void try_logging()
{
    BOOST_LOG_FUNCTION();

    src::severity_logger< >& lg = test_lg::get();

    BOOST_LOG_SEV(lg, critical) << "This is a critical severity record";

    BOOST_LOG_NAMED_SCOPE("random name");
    BOOST_LOG_SEV(lg, error) << "This is a error severity record";
}

int main(int argc, char* argv[])
{
    try
    {
        init_logging();
        try_logging();
    }
    catch (std::exception& e)
    {
        std::cout << "FAILURE: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
