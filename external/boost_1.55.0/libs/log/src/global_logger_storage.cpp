/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   global_logger_storage.cpp
 * \author Andrey Semashev
 * \date   21.04.2008
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

#include <map>
#include <string>
#include <boost/limits.hpp>
#include <boost/log/exceptions.hpp>
#include <boost/log/detail/snprintf.hpp>
#include <boost/log/detail/singleton.hpp>
#include <boost/log/utility/type_info_wrapper.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#if !defined(BOOST_LOG_NO_THREADS)
#include <boost/thread/mutex.hpp>
#include <boost/log/detail/locks.hpp>
#endif
#include <boost/log/detail/header.hpp>

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace sources {

namespace aux {

BOOST_LOG_ANONYMOUS_NAMESPACE {

//! The loggers repository singleton
struct loggers_repository :
    public log::aux::lazy_singleton< loggers_repository >
{
    //! Repository map type
    typedef std::map< type_info_wrapper, shared_ptr< logger_holder_base > > loggers_map_t;

#if !defined(BOOST_LOG_NO_THREADS)
    //! Synchronization primitive
    mutable mutex m_Mutex;
#endif
    //! Map of logger holders
    loggers_map_t m_Loggers;
};

} // namespace

//! Finds or creates the logger and returns its holder
BOOST_LOG_API shared_ptr< logger_holder_base > global_storage::get_or_init(std::type_info const& key, initializer_t initializer)
{
    typedef loggers_repository::loggers_map_t loggers_map_t;
    loggers_repository& repo = loggers_repository::get();
    type_info_wrapper wrapped_key = key;

    BOOST_LOG_EXPR_IF_MT(log::aux::exclusive_lock_guard< mutex > lock(repo.m_Mutex);)
    loggers_map_t::iterator it = repo.m_Loggers.find(wrapped_key);
    if (it != repo.m_Loggers.end())
    {
        // There is an instance
        return it->second;
    }
    else
    {
        // We have to create a logger instance
        shared_ptr< logger_holder_base > inst = initializer();
        repo.m_Loggers[wrapped_key] = inst;
        return inst;
    }
}

//! Throws the \c odr_violation exception
BOOST_LOG_API BOOST_LOG_NORETURN void throw_odr_violation(
    std::type_info const& tag_type,
    std::type_info const& logger_type,
    logger_holder_base const& registered)
{
    char buf[std::numeric_limits< unsigned int >::digits10 + 3];
    log::aux::snprintf(buf, sizeof(buf), "%u", registered.m_RegistrationLine);
    std::string str =
        std::string("Could not initialize global logger with tag \"") +
        type_info_wrapper(tag_type).pretty_name() +
        "\" and type \"" +
        type_info_wrapper(logger_type).pretty_name() +
        "\". A logger of type \"" +
        type_info_wrapper(registered.logger_type()).pretty_name() +
        "\" with the same tag has already been registered at " +
        registered.m_RegistrationFile + ":" + buf + ".";

    BOOST_LOG_THROW_DESCR(odr_violation, str);
}

} // namespace aux

} // namespace sources

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>
