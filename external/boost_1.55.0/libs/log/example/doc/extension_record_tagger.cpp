/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <string>
#include <ostream>
#include <fstream>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/scope_exit.hpp>
#include <boost/mpl/quote.hpp>
#include <boost/parameter/keyword.hpp>
#include <boost/thread/locks.hpp>
#include <boost/move/utility.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes/constant.hpp>
#include <boost/log/sources/features.hpp>
#include <boost/log/sources/basic_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/utility/strictest_lock.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;
namespace keywords = boost::log::keywords;

//[ example_extension_record_tagger_keyword
namespace my_keywords {

    BOOST_PARAMETER_KEYWORD(tag_ns, tag)

}
//]

//[ example_extension_record_tagger_declaration
template< typename BaseT >
class record_tagger_feature :
    public BaseT                        /*< the feature should derive from other features or the basic_logger class >*/
{
public:
    // Let's import some types that we will need. These imports should be public,
    // in order to allow other features that may derive from record_tagger to do the same.
    typedef typename BaseT::char_type char_type;
    typedef typename BaseT::threading_model threading_model;

public:
    // Default constructor. Initializes m_Tag to an invalid value.
    record_tagger_feature();
    // Copy constructor. Initializes m_Tag to a value, equivalent to that.m_Tag.
    record_tagger_feature(record_tagger_feature const& that);
    // Forwarding constructor with named parameters
    template< typename ArgsT >
    record_tagger_feature(ArgsT const& args);

    // The method will require locking, so we have to define locking requirements for it.
    // We use the strictest_lock trait in order to choose the most restricting lock type.
    typedef typename logging::strictest_lock<
        boost::lock_guard< threading_model >,
        typename BaseT::open_record_lock,
        typename BaseT::add_attribute_lock,
        typename BaseT::remove_attribute_lock
    >::type open_record_lock;

protected:
    // Lock-less implementation of operations
    template< typename ArgsT >
    logging::record open_record_unlocked(ArgsT const& args);
};

// A convenience metafunction to specify the feature
// in the list of features of the final logger later
struct record_tagger :
    public boost::mpl::quote1< record_tagger_feature >
{
};
//]

//[ example_extension_record_tagger_structors
template< typename BaseT >
record_tagger_feature< BaseT >::record_tagger_feature()
{
}

template< typename BaseT >
record_tagger_feature< BaseT >::record_tagger_feature(record_tagger_feature const& that) :
    BaseT(static_cast< BaseT const& >(that))
{
}

template< typename BaseT >
template< typename ArgsT >
record_tagger_feature< BaseT >::record_tagger_feature(ArgsT const& args) : BaseT(args)
{
}
//]

//[ example_extension_record_tagger_open_record
template< typename BaseT >
template< typename ArgsT >
logging::record record_tagger_feature< BaseT >::open_record_unlocked(ArgsT const& args)
{
    // Extract the named argument from the parameters pack
    std::string tag_value = args[my_keywords::tag | std::string()];

    logging::attribute_set& attrs = BaseT::attributes();
    logging::attribute_set::iterator tag = attrs.end();
    if (!tag_value.empty())
    {
        // Add the tag as a new attribute
        std::pair<
            logging::attribute_set::iterator,
            bool
        > res = BaseT::add_attribute_unlocked("Tag",
            attrs::constant< std::string >(tag_value));
        if (res.second)
            tag = res.first;
    }

    // In any case, after opening a record remove the tag from the attributes
    BOOST_SCOPE_EXIT_TPL((&tag)(&attrs))
    {
        if (tag != attrs.end())
            attrs.erase(tag);
    }
    BOOST_SCOPE_EXIT_END

    // Forward the call to the base feature
    return BaseT::open_record_unlocked(args);
}
//]

//[ example_extension_record_tagger_my_logger
template< typename LevelT = int >
class my_logger :
    public src::basic_composite_logger<
        char,                           /*< character type for the logger >*/
        my_logger< LevelT >,            /*< final logger type >*/
        src::single_thread_model,       /*< the logger does not perform thread synchronization; use `multi_thread_model` to declare a thread-safe logger >*/
        src::features<                  /*< the list of features we want to combine >*/
            src::severity< LevelT >,
            record_tagger
        >
    >
{
    // The following line will automatically generate forwarding constructors that
    // will call to the corresponding constructors of the base class
    BOOST_LOG_FORWARD_LOGGER_MEMBERS_TEMPLATE(my_logger)
};
//]

//[ example_extension_record_tagger_severity
enum severity_level
{
    normal,
    warning,
    error
};
//]

inline std::ostream& operator<< (std::ostream& strm, severity_level level)
{
    const char* levels[] =
    {
        "normal",
        "warning",
        "error"
    };

    if (static_cast< std::size_t >(level) < sizeof(levels) / sizeof(*levels))
        strm << levels[level];
    else
        strm << static_cast< int >(level);

    return strm;
}

//[ example_extension_record_tagger_manual_logging
void manual_logging()
{
    my_logger< severity_level > logger;

    logging::record rec = logger.open_record((keywords::severity = normal, my_keywords::tag = "GUI"));
    if (rec)
    {
        logging::record_ostream strm(rec);
        strm << "The user has confirmed his choice";
        strm.flush();
        logger.push_record(boost::move(rec));
    }
}
//]

//[ example_extension_record_tagger_macro_logging
#define LOG_WITH_TAG(lg, sev, tg) \
    BOOST_LOG_WITH_PARAMS((lg), (keywords::severity = (sev))(my_keywords::tag = (tg)))

void logging_function()
{
    my_logger< severity_level > logger;

    LOG_WITH_TAG(logger, normal, "GUI") << "The user has confirmed his choice";
}
//]

void init()
{
    typedef sinks::synchronous_sink< sinks::text_ostream_backend > text_sink;
    boost::shared_ptr< text_sink > sink = boost::make_shared< text_sink >();

    sink->locked_backend()->add_stream(
        boost::make_shared< std::ofstream >("sample.log"));

    sink->set_formatter
    (
        expr::stream
            << expr::attr< unsigned int >("LineID")
            << ": <" << expr::attr< severity_level >("Severity")
            << "> [" << expr::attr< std::string >("Tag") << "]\t"
            << expr::smessage
    );

    logging::core::get()->add_sink(sink);

    // Add attributes
    logging::add_common_attributes();
}


int main(int, char*[])
{
    init();

    logging_function();
    manual_logging();

    return 0;
}
