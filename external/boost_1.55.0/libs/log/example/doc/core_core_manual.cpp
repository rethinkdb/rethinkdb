/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/move/utility.hpp>
#include <boost/log/core.hpp>
#include <boost/log/sources/record_ostream.hpp>

namespace logging = boost::log;

//[ example_core_core_manual_logging
void logging_function(logging::attribute_set const& attrs)
{
    boost::shared_ptr< logging::core > core = logging::core::get();

    // Attempt to open a log record
    logging::record rec = core->open_record(attrs);
    if (rec)
    {
        // Ok, the record is accepted. Compose the message now.
        logging::record_ostream strm(rec);
        strm << "Hello, World!";
        strm.flush();

        // Deliver the record to the sinks.
        core->push_record(boost::move(rec));
    }
}
//]

int main(int, char*[])
{
    logging_function(logging::attribute_set());

    return 0;
}
