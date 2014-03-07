/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   record_ostream.hpp
 * \author Andrey Semashev
 * \date   09.03.2009
 *
 * This header contains a wrapper class around a logging record that allows to compose the
 * record message with a streaming expression.
 */

#ifndef BOOST_LOG_SOURCES_RECORD_OSTREAM_HPP_INCLUDED_
#define BOOST_LOG_SOURCES_RECORD_OSTREAM_HPP_INCLUDED_

#include <string>
#include <ostream>
#include <boost/assert.hpp>
#include <boost/move/core.hpp>
#include <boost/move/utility.hpp>
#include <boost/utility/addressof.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/native_typeof.hpp>
#include <boost/log/detail/unhandled_exception_count.hpp>
#include <boost/log/core/record.hpp>
#include <boost/log/utility/unique_identifier_name.hpp>
#include <boost/utility/explicit_operator_bool.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

/*!
 * \brief Logging record adapter with a streaming capability
 *
 * This class allows to compose the logging record message by streaming operations. It
 * aggregates the log record and provides the standard output stream interface.
 */
template< typename CharT >
class basic_record_ostream :
    public basic_formatting_ostream< CharT >
{
    //! Self type
    typedef basic_record_ostream< CharT > this_type;
    //! Base stream class
    typedef basic_formatting_ostream< CharT > base_type;

public:
    //! Character type
    typedef CharT char_type;
    //! String type to be used as a message text holder
    typedef std::basic_string< char_type > string_type;
    //! Stream type
    typedef std::basic_ostream< char_type > stream_type;

private:
    //! Log record
    record* m_record;

public:
    /*!
     * Default constructor. Creates an empty record that is equivalent to the invalid record handle.
     * The stream capability is not available after construction.
     *
     * \post <tt>!*this == true</tt>
     */
    basic_record_ostream() BOOST_NOEXCEPT : m_record(NULL) {}

    /*!
     * Constructor from a record object. Attaches to the provided record.
     *
     * \pre <tt>!!rec == true</tt>
     * \post <tt>&this->get_record() == &rec</tt>
     * \param rec The record handle being attached to
     */
    explicit basic_record_ostream(record& rec)
    {
        BOOST_ASSERT_MSG(!!rec, "Boost.Log: basic_record_ostream should only be attached to a valid record");
        m_record = &rec;
        init_stream();
    }

    /*!
     * Destructor. Destroys the record, releases any sinks and attribute values that were involved in processing this record.
     */
    ~basic_record_ostream() BOOST_NOEXCEPT
    {
        detach_from_record();
    }

    /*!
     * Conversion to an unspecified boolean type
     *
     * \return \c true, if stream is valid and ready for formatting, \c false, if the stream is not valid. The latter also applies to
     *         the case when the stream is not attached to a log record.
     */
    BOOST_EXPLICIT_OPERATOR_BOOL()

    /*!
     * Inverted conversion to an unspecified boolean type
     *
     * \return \c false, if stream is valid and ready for formatting, \c true, if the stream is not valid. The latter also applies to
     *         the case when the stream is not attached to a log record.
     */
    bool operator! () const BOOST_NOEXCEPT
    {
        return (!m_record || base_type::fail());
    }

    /*!
     * Flushes internal buffers to complete all pending formatting operations and returns the aggregated log record
     *
     * \return The aggregated record object
     */
    record& get_record()
    {
        BOOST_ASSERT(m_record != NULL);
        this->flush();
        return *m_record;
    }

    /*!
     * Flushes internal buffers to complete all pending formatting operations and returns the aggregated log record
     *
     * \return The aggregated record object
     */
    record const& get_record() const
    {
        BOOST_ASSERT(m_record != NULL);
        const_cast< this_type* >(this)->flush();
        return *m_record;
    }

    /*!
     * If the stream is attached to a log record, flushes internal buffers to complete all pending formatting operations.
     * Then reattaches the stream to another log record.
     *
     * \param rec New log record to attach to
     */
    void attach_record(record& rec)
    {
        BOOST_ASSERT_MSG(!!rec, "Boost.Log: basic_record_ostream should only be attached to a valid record");
        detach_from_record();
        m_record = &rec;
        init_stream();
    }

    //! The function resets the stream into a detached (default initialized) state
    BOOST_LOG_API void detach_from_record() BOOST_NOEXCEPT;

private:
    //! The function initializes the stream and the stream buffer
    BOOST_LOG_API void init_stream();

    //  Copy and assignment are closed
    BOOST_DELETED_FUNCTION(basic_record_ostream(basic_record_ostream const&))
    BOOST_DELETED_FUNCTION(basic_record_ostream& operator= (basic_record_ostream const&))
};


#ifdef BOOST_LOG_USE_CHAR
typedef basic_record_ostream< char > record_ostream;        //!< Convenience typedef for narrow-character logging
#endif
#ifdef BOOST_LOG_USE_WCHAR_T
typedef basic_record_ostream< wchar_t > wrecord_ostream;    //!< Convenience typedef for wide-character logging
#endif

namespace aux {

//! Internal class that provides formatting streams for record pumps
template< typename CharT >
struct stream_provider
{
    //! Character type
    typedef CharT char_type;

    //! Formatting stream compound
    struct stream_compound
    {
        stream_compound* next;

        //! Log record stream adapter
        basic_record_ostream< char_type > stream;

        //! Initializing constructor
        explicit stream_compound(record& rec) : next(NULL), stream(rec) {}
    };

    //! The method returns an allocated stream compound
    BOOST_LOG_API static stream_compound* allocate_compound(record& rec);
    //! The method releases a compound
    BOOST_LOG_API static void release_compound(stream_compound* compound) BOOST_NOEXCEPT;

    //  Non-constructible, non-copyable, non-assignable
    BOOST_DELETED_FUNCTION(stream_provider())
    BOOST_DELETED_FUNCTION(stream_provider(stream_provider const&))
    BOOST_DELETED_FUNCTION(stream_provider& operator= (stream_provider const&))
};


/*!
 * \brief Logging record pump implementation
 *
 * The pump is used to format the logging record message text and then
 * push it to the logging core. It is constructed on each attempt to write
 * a log record and destroyed afterwards.
 *
 * The pump class template is instantiated on the logger type.
 */
template< typename LoggerT >
class record_pump
{
    BOOST_MOVABLE_BUT_NOT_COPYABLE(record_pump)

private:
    //! Logger type
    typedef LoggerT logger_type;
    //! Character type
    typedef typename logger_type::char_type char_type;
    //! Stream compound provider
    typedef stream_provider< char_type > stream_provider_type;
    //! Stream compound type
    typedef typename stream_provider_type::stream_compound stream_compound;

    //! Stream compound release guard
    class auto_release;
    friend class auto_release;
    class auto_release
    {
        stream_compound* m_pCompound;

    public:
        explicit auto_release(stream_compound* p) BOOST_NOEXCEPT : m_pCompound(p) {}
        ~auto_release() BOOST_NOEXCEPT { stream_provider_type::release_compound(m_pCompound); }
    };

protected:
    //! A reference to the logger
    logger_type* m_pLogger;
    //! Stream compound
    stream_compound* m_pStreamCompound;
    //! Exception state
    const unsigned int m_ExceptionCount;

public:
    //! Constructor
    explicit record_pump(logger_type& lg, record& rec) :
        m_pLogger(boost::addressof(lg)),
        m_pStreamCompound(stream_provider_type::allocate_compound(rec)),
        m_ExceptionCount(unhandled_exception_count())
    {
    }
    //! Move constructor
    record_pump(BOOST_RV_REF(record_pump) that) BOOST_NOEXCEPT :
        m_pLogger(that.m_pLogger),
        m_pStreamCompound(that.m_pStreamCompound),
        m_ExceptionCount(that.m_ExceptionCount)
    {
        that.m_pLogger = 0;
        that.m_pStreamCompound = 0;
    }
    //! Destructor. Pushes the composed message to log.
    ~record_pump() BOOST_NOEXCEPT_IF(false)
    {
        if (m_pLogger)
        {
            auto_release cleanup(m_pStreamCompound); // destructor doesn't throw
            // Only push the record if no exception has been thrown in the streaming expression (if possible)
            if (m_ExceptionCount >= unhandled_exception_count())
                m_pLogger->push_record(boost::move(m_pStreamCompound->stream.get_record()));
        }
    }

    //! Returns the stream to be used for message text formatting
    basic_record_ostream< char_type >& stream() const BOOST_NOEXCEPT
    {
        BOOST_ASSERT(m_pStreamCompound != 0);
        return m_pStreamCompound->stream;
    }
};

template< typename LoggerT >
BOOST_FORCEINLINE record_pump< LoggerT > make_record_pump(LoggerT& lg, record& rec)
{
    return record_pump< LoggerT >(lg, rec);
}

} // namespace aux

#ifndef BOOST_LOG_DOXYGEN_PASS

#define BOOST_LOG_STREAM_INTERNAL(logger, rec_var)\
    for (::boost::log::record rec_var = (logger).open_record(); !!rec_var;)\
        ::boost::log::aux::make_record_pump((logger), rec_var).stream()

#define BOOST_LOG_STREAM_WITH_PARAMS_INTERNAL(logger, rec_var, params_seq)\
    for (::boost::log::record rec_var = (logger).open_record((BOOST_PP_SEQ_ENUM(params_seq))); !!rec_var;)\
        ::boost::log::aux::make_record_pump((logger), rec_var).stream()

#endif // BOOST_LOG_DOXYGEN_PASS

//! The macro writes a record to the log
#define BOOST_LOG_STREAM(logger)\
    BOOST_LOG_STREAM_INTERNAL(logger, BOOST_LOG_UNIQUE_IDENTIFIER_NAME(_boost_log_record_))

//! The macro writes a record to the log and allows to pass additional named arguments to the logger
#define BOOST_LOG_STREAM_WITH_PARAMS(logger, params_seq)\
    BOOST_LOG_STREAM_WITH_PARAMS_INTERNAL(logger, BOOST_LOG_UNIQUE_IDENTIFIER_NAME(_boost_log_record_), params_seq)

#ifndef BOOST_LOG_NO_SHORTHAND_NAMES

//! An equivalent to BOOST_LOG_STREAM(logger)
#define BOOST_LOG(logger) BOOST_LOG_STREAM(logger)

//! An equivalent to BOOST_LOG_STREAM_WITH_PARAMS(logger, params_seq)
#define BOOST_LOG_WITH_PARAMS(logger, params_seq) BOOST_LOG_STREAM_WITH_PARAMS(logger, params_seq)

#endif // BOOST_LOG_NO_SHORTHAND_NAMES

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_SOURCES_RECORD_OSTREAM_HPP_INCLUDED_
