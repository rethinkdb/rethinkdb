/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   record_emission.cpp
 * \author Andrey Semashev
 * \date   22.03.2009
 *
 * \brief  This code measures performance of log record emission
 */

// #define BOOST_LOG_USE_CHAR
// #define BOOST_ALL_DYN_LINK 1
// #define BOOST_LOG_DYN_LINK 1
#define BOOST_NO_DYN_LINK 1

#include <iomanip>
#include <iostream>
#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/date_time/microsec_time_clock.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/barrier.hpp>

#include <boost/log/core.hpp>
#include <boost/log/common.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/sinks/basic_sink_backend.hpp>

#include <boost/log/expressions.hpp>

#include <boost/log/attributes/scoped_attribute.hpp>

enum config
{
    RECORD_COUNT = 50000000,
    THREAD_COUNT = 3,
    SINK_COUNT = 3
};

namespace logging = boost::log;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace keywords = boost::log::keywords;

enum severity_level
{
    normal,
    warning,
    error
};

BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", severity_level)

namespace {

    //! A fake sink backend that receives log records
    class fake_backend :
        public sinks::basic_sink_backend< sinks::concurrent_feeding >
    {
    public:
        void consume(logging::record_view const& rec)
        {
        }
    };

} // namespace

void test(unsigned int record_count, boost::barrier& bar)
{
    BOOST_LOG_SCOPED_THREAD_TAG("ThreadID", boost::this_thread::get_id());
    bar.wait();

    src::severity_logger< severity_level > slg;
    for (unsigned int i = 0; i < record_count; ++i)
    {
        BOOST_LOG_SEV(slg, warning) << "Test record";
    }
}

int main(int argc, char* argv[])
{
    std::cout << "Test config: " << THREAD_COUNT << " threads, " << SINK_COUNT << " sinks, " << RECORD_COUNT << " records" << std::endl;
//__debugbreak();
//    typedef sinks::unlocked_sink< fake_backend > fake_sink;
//    typedef sinks::synchronous_sink< fake_backend > fake_sink;
    typedef sinks::asynchronous_sink< fake_backend > fake_sink;
    for (unsigned int i = 0; i < SINK_COUNT; ++i)
        logging::core::get()->add_sink(boost::make_shared< fake_sink >());

    logging::core::get()->add_global_attribute("LineID", attrs::counter< unsigned int >(1));
    logging::core::get()->add_global_attribute("TimeStamp", attrs::local_clock());
    logging::core::get()->add_global_attribute("Scope", attrs::named_scope());

//    logging::core::get()->set_filter(severity > normal); // all records pass the filter
//    logging::core::get()->set_filter(severity > error); // all records don't pass the filter

    logging::core::get()->set_filter(severity > error); // all records don't pass the filter

    const unsigned int record_count = RECORD_COUNT / THREAD_COUNT;
    boost::barrier bar(THREAD_COUNT);
    boost::thread_group threads;

    for (unsigned int i = 1; i < THREAD_COUNT; ++i)
        threads.create_thread(boost::bind(&test, record_count, boost::ref(bar)));

    boost::posix_time::ptime start = boost::date_time::microsec_clock< boost::posix_time::ptime >::universal_time(), end;
    test(record_count, bar);
    if (THREAD_COUNT > 1)
        threads.join_all();
    end = boost::date_time::microsec_clock< boost::posix_time::ptime >::universal_time();

    unsigned long long duration = (end - start).total_microseconds();

    std::cout << "Test duration: " << duration << " us ("
        << std::fixed << std::setprecision(3) << static_cast< double >(RECORD_COUNT) / (static_cast< double >(duration) / 1000000.0)
        << " records per second)" << std::endl;

    return 0;
}
