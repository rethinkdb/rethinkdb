/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <string>
#include <iostream>
#include <stdexcept>
#include <boost/config.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/utility/empty_deleter.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/attributes/attribute.hpp>
#include <boost/log/attributes/attribute_value.hpp>
#include <boost/log/attributes/attribute_value_impl.hpp>
#include <boost/log/attributes/attribute_cast.hpp>

// Includes for get_uptime()
#include <boost/throw_exception.hpp>
#if defined(BOOST_WINDOWS)
#include <windows.h>
#elif defined(__linux__) || defined(__linux) || defined(linux)
#include <sys/sysinfo.h>
#elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
#include <time.h>
#include <errno.h>
#include <sys/sysctl.h>
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
#include <time.h>
#endif

namespace logging = boost::log;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;

//[ example_extension_system_uptime_attr_impl
// The function returns the system uptime, in seconds
unsigned int get_uptime();

// Attribute implementation class
class system_uptime_impl :
    public logging::attribute::impl
{
public:
    // The method generates a new attribute value
    logging::attribute_value get_value()
    {
        return attrs::make_attribute_value(get_uptime());
    }
};
//]

//[ example_extension_system_uptime_attr
// Attribute interface class
class system_uptime :
    public logging::attribute
{
public:
    system_uptime() : logging::attribute(new system_uptime_impl())
    {
    }
    // Attribute casting support
    explicit system_uptime(attrs::cast_source const& source) : logging::attribute(source.as< system_uptime_impl >())
    {
    }
};
//]

//[ example_extension_system_uptime_use
void init_logging()
{
    boost::shared_ptr< logging::core > core = logging::core::get();

    //<-
    // Initialize the sink
    typedef sinks::synchronous_sink< sinks::text_ostream_backend > sink_t;
    boost::shared_ptr< sink_t > sink(new sink_t());
    sink->locked_backend()->add_stream(boost::shared_ptr< std::ostream >(&std::clog, boost::empty_deleter()));
    sink->set_formatter(expr::stream << expr::attr< unsigned int >("SystemUptime") << ": " << expr::smessage);
    core->add_sink(sink);
    //->
    // ...

    // Add the uptime attribute to the core
    core->add_global_attribute("SystemUptime", system_uptime());
}
//]

unsigned int get_uptime()
{
#if defined(BOOST_WINDOWS)
    return GetTickCount() / 1000u;
#elif defined(__linux__) || defined(__linux) || defined(linux)
    struct sysinfo info;
    if (sysinfo(&info) != 0)
        BOOST_THROW_EXCEPTION(std::runtime_error("Could not acquire uptime"));
    return info.uptime;
#elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
    struct timeval boottime;
    std::size_t len = sizeof(boottime);
    int mib[2] = { CTL_KERN, KERN_BOOTTIME };
    if (sysctl(mib, 2, &boottime, &len, NULL, 0) < 0)
        BOOST_THROW_EXCEPTION(std::runtime_error("Could not acquire uptime"));
    return time(NULL) - boottime.tv_sec;
#elif (defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)) && defined(CLOCK_UPTIME)
    struct timespec ts;
    if (clock_gettime(CLOCK_UPTIME, &ts) != 0)
        BOOST_THROW_EXCEPTION(std::runtime_error("Could not acquire uptime"));
    return ts.tv_sec;
#else
    return 0;
#endif
}

int main(int, char*[])
{
    init_logging();

    src::logger lg;
    BOOST_LOG(lg) << "Hello, world with uptime!";

    return 0;
}
