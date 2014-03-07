/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <cstdlib>
#include <string>
#include <utility>
#include <stdexcept>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/phoenix.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes/current_process_name.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/basic_sink_backend.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/value_ref.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>

namespace logging = boost::log;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;

//[ example_extension_app_launcher_definition
// The backend starts an external application to display notifications
class app_launcher :
    public sinks::basic_formatted_sink_backend<
        char,                                                                   /*< target character type >*/
        sinks::synchronized_feeding                                             /*< in order not to spawn too many application instances we require records to be processed serial >*/
    >
{
public:
    // The function consumes the log records that come from the frontend
    void consume(logging::record_view const& rec, string_type const& command_line);
};
//]

//[ example_extension_app_launcher_consume
// The function consumes the log records that come from the frontend
void app_launcher::consume(logging::record_view const& rec, string_type const& command_line)
{
    std::system(command_line.c_str());
}
//]

//[ example_extension_app_launcher_formatting
BOOST_LOG_ATTRIBUTE_KEYWORD(process_name, "ProcessName", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(caption, "Caption", std::string)

// Custom severity level formatting function
std::string severity_level_as_urgency(
    logging::value_ref< logging::trivial::severity_level, logging::trivial::tag::severity > const& level)
{
    if (!level || level.get() == logging::trivial::info)
        return "normal";
    logging::trivial::severity_level lvl = level.get();
    if (lvl < logging::trivial::info)
        return "low";
    else
        return "critical";
}

// The function initializes the logging library
void init_logging()
{
    boost::shared_ptr< logging::core > core = logging::core::get();

    typedef sinks::synchronous_sink< app_launcher > sink_t;
    boost::shared_ptr< sink_t > sink(new sink_t());

    const std::pair< const char*, const char* > shell_decorations[] =
    {
        std::pair< const char*, const char* >("\"", "\\\""),
        std::pair< const char*, const char* >("$", "\\$"),
        std::pair< const char*, const char* >("!", "\\!")
    };

    // Make the formatter generate the command line for notify-send
    sink->set_formatter
    (
        expr::stream << "notify-send -t 2000 -u "
            << boost::phoenix::bind(&severity_level_as_urgency, logging::trivial::severity.or_none())
            << expr::if_(expr::has_attr(process_name))
               [
                    expr::stream << " -a '" << process_name << "'"
               ]
            << expr::if_(expr::has_attr(caption))
               [
                    expr::stream << " \"" << expr::char_decor(shell_decorations)[ expr::stream << caption ] << "\""
               ]
            << " \"" << expr::char_decor(shell_decorations)[ expr::stream << expr::message ] << "\""
    );

    core->add_sink(sink);

    // Add attributes that we will use
    core->add_global_attribute("ProcessName", attrs::current_process_name());
}
//]

//[ example_extension_app_launcher_logging
void test_notifications()
{
    BOOST_LOG_TRIVIAL(debug) << "Hello, it's a simple notification";
    BOOST_LOG_TRIVIAL(info) << logging::add_value(caption, "Caption text") << "And this notification has caption as well";
}
//]

int main(int, char*[])
{
    init_logging();
    test_notifications();

    return 0;
}
