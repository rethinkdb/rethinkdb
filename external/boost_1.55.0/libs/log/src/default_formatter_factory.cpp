/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   default_formatter_factory.hpp
 * \author Andrey Semashev
 * \date   14.07.2013
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

#if !defined(BOOST_LOG_WITHOUT_SETTINGS_PARSERS) && !defined(BOOST_LOG_WITHOUT_DEFAULT_FACTORIES)

#undef BOOST_MPL_LIMIT_VECTOR_SIZE
#define BOOST_MPL_LIMIT_VECTOR_SIZE 50

#include <boost/mpl/vector.hpp>
#include <boost/mpl/copy.hpp>
#include <boost/mpl/push_back.hpp>
#include <boost/mpl/back_inserter.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/operator.hpp>
#include <boost/log/exceptions.hpp>
#include <boost/log/expressions/attr.hpp>
#include <boost/log/expressions/formatters/stream.hpp>
#include <boost/log/utility/type_dispatch/standard_types.hpp>
#include <boost/log/utility/type_dispatch/date_time_types.hpp>
#include <boost/log/utility/string_literal.hpp>
#include <boost/log/detail/code_conversion.hpp>
#include <boost/log/detail/process_id.hpp>
#if !defined(BOOST_LOG_NO_THREADS)
#include <boost/log/detail/thread_id.hpp>
#endif
#include <boost/log/attributes/named_scope.hpp>
#include "default_formatter_factory.hpp"
#include <boost/log/detail/header.hpp>

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

//! The callback for equality relation filter
template< typename CharT >
typename default_formatter_factory< CharT >::formatter_type
default_formatter_factory< CharT >::create_formatter(attribute_name const& name, args_map const& args)
{
    // No user-defined factory, shall use the most generic formatter we can ever imagine at this point
    typedef mpl::copy<
        // We have to exclude std::time_t since it's an integral type and will conflict with one of the standard types
        boost_time_period_types,
        mpl::back_inserter<
            mpl::copy<
                boost_time_duration_types,
                mpl::back_inserter< boost_date_time_types >
            >::type
        >
    >::type time_related_types;

    typedef mpl::copy<
        mpl::copy<
            mpl::vector<
                attributes::named_scope_list,
#if !defined(BOOST_LOG_NO_THREADS)
                log::aux::thread::id,
#endif
                log::aux::process::id
            >,
            mpl::back_inserter< time_related_types >
        >::type,
        mpl::back_inserter< default_attribute_types >
    >::type supported_types;

    return formatter_type(expressions::stream << expressions::attr< supported_types::type >(name));
}

//  Explicitly instantiate factory implementation
#ifdef BOOST_LOG_USE_CHAR
template class default_formatter_factory< char >;
#endif
#ifdef BOOST_LOG_USE_WCHAR_T
template class default_formatter_factory< wchar_t >;
#endif

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // !defined(BOOST_LOG_WITHOUT_SETTINGS_PARSERS) && !defined(BOOST_LOG_WITHOUT_DEFAULT_FACTORIES)
