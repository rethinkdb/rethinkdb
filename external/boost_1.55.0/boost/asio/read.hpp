//
// read.hpp
// ~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_READ_HPP
#define BOOST_ASIO_READ_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <cstddef>
#include <boost/asio/async_result.hpp>
#include <boost/asio/basic_streambuf_fwd.hpp>
#include <boost/asio/error.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

/**
 * @defgroup read boost::asio::read
 *
 * @brief Attempt to read a certain amount of data from a stream before
 * returning.
 */
/*@{*/

/// Attempt to read a certain amount of data from a stream before returning.
/**
 * This function is used to read a certain number of bytes of data from a
 * stream. The call will block until one of the following conditions is true:
 *
 * @li The supplied buffers are full. That is, the bytes transferred is equal to
 * the sum of the buffer sizes.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * read_some function.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the SyncReadStream concept.
 *
 * @param buffers One or more buffers into which the data will be read. The sum
 * of the buffer sizes indicates the maximum number of bytes to read from the
 * stream.
 *
 * @returns The number of bytes transferred.
 *
 * @throws boost::system::system_error Thrown on failure.
 *
 * @par Example
 * To read into a single data buffer use the @ref buffer function as follows:
 * @code boost::asio::read(s, boost::asio::buffer(data, size)); @endcode
 * See the @ref buffer documentation for information on reading into multiple
 * buffers in one go, and how to use it with arrays, boost::array or
 * std::vector.
 *
 * @note This overload is equivalent to calling:
 * @code boost::asio::read(
 *     s, buffers,
 *     boost::asio::transfer_all()); @endcode
 */
template <typename SyncReadStream, typename MutableBufferSequence>
std::size_t read(SyncReadStream& s, const MutableBufferSequence& buffers);

/// Attempt to read a certain amount of data from a stream before returning.
/**
 * This function is used to read a certain number of bytes of data from a
 * stream. The call will block until one of the following conditions is true:
 *
 * @li The supplied buffers are full. That is, the bytes transferred is equal to
 * the sum of the buffer sizes.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * read_some function.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the SyncReadStream concept.
 *
 * @param buffers One or more buffers into which the data will be read. The sum
 * of the buffer sizes indicates the maximum number of bytes to read from the
 * stream.
 *
 * @param ec Set to indicate what error occurred, if any.
 *
 * @returns The number of bytes transferred.
 *
 * @par Example
 * To read into a single data buffer use the @ref buffer function as follows:
 * @code boost::asio::read(s, boost::asio::buffer(data, size), ec); @endcode
 * See the @ref buffer documentation for information on reading into multiple
 * buffers in one go, and how to use it with arrays, boost::array or
 * std::vector.
 *
 * @note This overload is equivalent to calling:
 * @code boost::asio::read(
 *     s, buffers,
 *     boost::asio::transfer_all(), ec); @endcode
 */
template <typename SyncReadStream, typename MutableBufferSequence>
std::size_t read(SyncReadStream& s, const MutableBufferSequence& buffers,
    boost::system::error_code& ec);

/// Attempt to read a certain amount of data from a stream before returning.
/**
 * This function is used to read a certain number of bytes of data from a
 * stream. The call will block until one of the following conditions is true:
 *
 * @li The supplied buffers are full. That is, the bytes transferred is equal to
 * the sum of the buffer sizes.
 *
 * @li The completion_condition function object returns 0.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * read_some function.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the SyncReadStream concept.
 *
 * @param buffers One or more buffers into which the data will be read. The sum
 * of the buffer sizes indicates the maximum number of bytes to read from the
 * stream.
 *
 * @param completion_condition The function object to be called to determine
 * whether the read operation is complete. The signature of the function object
 * must be:
 * @code std::size_t completion_condition(
 *   // Result of latest read_some operation.
 *   const boost::system::error_code& error,
 *
 *   // Number of bytes transferred so far.
 *   std::size_t bytes_transferred
 * ); @endcode
 * A return value of 0 indicates that the read operation is complete. A non-zero
 * return value indicates the maximum number of bytes to be read on the next
 * call to the stream's read_some function.
 *
 * @returns The number of bytes transferred.
 *
 * @throws boost::system::system_error Thrown on failure.
 *
 * @par Example
 * To read into a single data buffer use the @ref buffer function as follows:
 * @code boost::asio::read(s, boost::asio::buffer(data, size),
 *     boost::asio::transfer_at_least(32)); @endcode
 * See the @ref buffer documentation for information on reading into multiple
 * buffers in one go, and how to use it with arrays, boost::array or
 * std::vector.
 */
template <typename SyncReadStream, typename MutableBufferSequence,
  typename CompletionCondition>
std::size_t read(SyncReadStream& s, const MutableBufferSequence& buffers,
    CompletionCondition completion_condition);

/// Attempt to read a certain amount of data from a stream before returning.
/**
 * This function is used to read a certain number of bytes of data from a
 * stream. The call will block until one of the following conditions is true:
 *
 * @li The supplied buffers are full. That is, the bytes transferred is equal to
 * the sum of the buffer sizes.
 *
 * @li The completion_condition function object returns 0.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * read_some function.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the SyncReadStream concept.
 *
 * @param buffers One or more buffers into which the data will be read. The sum
 * of the buffer sizes indicates the maximum number of bytes to read from the
 * stream.
 *
 * @param completion_condition The function object to be called to determine
 * whether the read operation is complete. The signature of the function object
 * must be:
 * @code std::size_t completion_condition(
 *   // Result of latest read_some operation.
 *   const boost::system::error_code& error,
 *
 *   // Number of bytes transferred so far.
 *   std::size_t bytes_transferred
 * ); @endcode
 * A return value of 0 indicates that the read operation is complete. A non-zero
 * return value indicates the maximum number of bytes to be read on the next
 * call to the stream's read_some function.
 *
 * @param ec Set to indicate what error occurred, if any.
 *
 * @returns The number of bytes read. If an error occurs, returns the total
 * number of bytes successfully transferred prior to the error.
 */
template <typename SyncReadStream, typename MutableBufferSequence,
    typename CompletionCondition>
std::size_t read(SyncReadStream& s, const MutableBufferSequence& buffers,
    CompletionCondition completion_condition, boost::system::error_code& ec);

#if !defined(BOOST_ASIO_NO_IOSTREAM)

/// Attempt to read a certain amount of data from a stream before returning.
/**
 * This function is used to read a certain number of bytes of data from a
 * stream. The call will block until one of the following conditions is true:
 *
 * @li The supplied buffer is full (that is, it has reached maximum size).
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * read_some function.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the SyncReadStream concept.
 *
 * @param b The basic_streambuf object into which the data will be read.
 *
 * @returns The number of bytes transferred.
 *
 * @throws boost::system::system_error Thrown on failure.
 *
 * @note This overload is equivalent to calling:
 * @code boost::asio::read(
 *     s, b,
 *     boost::asio::transfer_all()); @endcode
 */
template <typename SyncReadStream, typename Allocator>
std::size_t read(SyncReadStream& s, basic_streambuf<Allocator>& b);

/// Attempt to read a certain amount of data from a stream before returning.
/**
 * This function is used to read a certain number of bytes of data from a
 * stream. The call will block until one of the following conditions is true:
 *
 * @li The supplied buffer is full (that is, it has reached maximum size).
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * read_some function.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the SyncReadStream concept.
 *
 * @param b The basic_streambuf object into which the data will be read.
 *
 * @param ec Set to indicate what error occurred, if any.
 *
 * @returns The number of bytes transferred.
 *
 * @note This overload is equivalent to calling:
 * @code boost::asio::read(
 *     s, b,
 *     boost::asio::transfer_all(), ec); @endcode
 */
template <typename SyncReadStream, typename Allocator>
std::size_t read(SyncReadStream& s, basic_streambuf<Allocator>& b,
    boost::system::error_code& ec);

/// Attempt to read a certain amount of data from a stream before returning.
/**
 * This function is used to read a certain number of bytes of data from a
 * stream. The call will block until one of the following conditions is true:
 *
 * @li The supplied buffer is full (that is, it has reached maximum size).
 *
 * @li The completion_condition function object returns 0.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * read_some function.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the SyncReadStream concept.
 *
 * @param b The basic_streambuf object into which the data will be read.
 *
 * @param completion_condition The function object to be called to determine
 * whether the read operation is complete. The signature of the function object
 * must be:
 * @code std::size_t completion_condition(
 *   // Result of latest read_some operation.
 *   const boost::system::error_code& error,
 *
 *   // Number of bytes transferred so far.
 *   std::size_t bytes_transferred
 * ); @endcode
 * A return value of 0 indicates that the read operation is complete. A non-zero
 * return value indicates the maximum number of bytes to be read on the next
 * call to the stream's read_some function.
 *
 * @returns The number of bytes transferred.
 *
 * @throws boost::system::system_error Thrown on failure.
 */
template <typename SyncReadStream, typename Allocator,
    typename CompletionCondition>
std::size_t read(SyncReadStream& s, basic_streambuf<Allocator>& b,
    CompletionCondition completion_condition);

/// Attempt to read a certain amount of data from a stream before returning.
/**
 * This function is used to read a certain number of bytes of data from a
 * stream. The call will block until one of the following conditions is true:
 *
 * @li The supplied buffer is full (that is, it has reached maximum size).
 *
 * @li The completion_condition function object returns 0.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * read_some function.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the SyncReadStream concept.
 *
 * @param b The basic_streambuf object into which the data will be read.
 *
 * @param completion_condition The function object to be called to determine
 * whether the read operation is complete. The signature of the function object
 * must be:
 * @code std::size_t completion_condition(
 *   // Result of latest read_some operation.
 *   const boost::system::error_code& error,
 *
 *   // Number of bytes transferred so far.
 *   std::size_t bytes_transferred
 * ); @endcode
 * A return value of 0 indicates that the read operation is complete. A non-zero
 * return value indicates the maximum number of bytes to be read on the next
 * call to the stream's read_some function.
 *
 * @param ec Set to indicate what error occurred, if any.
 *
 * @returns The number of bytes read. If an error occurs, returns the total
 * number of bytes successfully transferred prior to the error.
 */
template <typename SyncReadStream, typename Allocator,
    typename CompletionCondition>
std::size_t read(SyncReadStream& s, basic_streambuf<Allocator>& b,
    CompletionCondition completion_condition, boost::system::error_code& ec);

#endif // !defined(BOOST_ASIO_NO_IOSTREAM)

/*@}*/
/**
 * @defgroup async_read boost::asio::async_read
 *
 * @brief Start an asynchronous operation to read a certain amount of data from
 * a stream.
 */
/*@{*/

/// Start an asynchronous operation to read a certain amount of data from a
/// stream.
/**
 * This function is used to asynchronously read a certain number of bytes of
 * data from a stream. The function call always returns immediately. The
 * asynchronous operation will continue until one of the following conditions is
 * true:
 *
 * @li The supplied buffers are full. That is, the bytes transferred is equal to
 * the sum of the buffer sizes.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * async_read_some function, and is known as a <em>composed operation</em>. The
 * program must ensure that the stream performs no other read operations (such
 * as async_read, the stream's async_read_some function, or any other composed
 * operations that perform reads) until this operation completes.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the AsyncReadStream concept.
 *
 * @param buffers One or more buffers into which the data will be read. The sum
 * of the buffer sizes indicates the maximum number of bytes to read from the
 * stream. Although the buffers object may be copied as necessary, ownership of
 * the underlying memory blocks is retained by the caller, which must guarantee
 * that they remain valid until the handler is called.
 *
 * @param handler The handler to be called when the read operation completes.
 * Copies will be made of the handler as required. The function signature of the
 * handler must be:
 * @code void handler(
 *   const boost::system::error_code& error, // Result of operation.
 *
 *   std::size_t bytes_transferred           // Number of bytes copied into the
 *                                           // buffers. If an error occurred,
 *                                           // this will be the  number of
 *                                           // bytes successfully transferred
 *                                           // prior to the error.
 * ); @endcode
 * Regardless of whether the asynchronous operation completes immediately or
 * not, the handler will not be invoked from within this function. Invocation of
 * the handler will be performed in a manner equivalent to using
 * boost::asio::io_service::post().
 *
 * @par Example
 * To read into a single data buffer use the @ref buffer function as follows:
 * @code
 * boost::asio::async_read(s, boost::asio::buffer(data, size), handler);
 * @endcode
 * See the @ref buffer documentation for information on reading into multiple
 * buffers in one go, and how to use it with arrays, boost::array or
 * std::vector.
 *
 * @note This overload is equivalent to calling:
 * @code boost::asio::async_read(
 *     s, buffers,
 *     boost::asio::transfer_all(),
 *     handler); @endcode
 */
template <typename AsyncReadStream, typename MutableBufferSequence,
    typename ReadHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler,
    void (boost::system::error_code, std::size_t))
async_read(AsyncReadStream& s, const MutableBufferSequence& buffers,
    BOOST_ASIO_MOVE_ARG(ReadHandler) handler);

/// Start an asynchronous operation to read a certain amount of data from a
/// stream.
/**
 * This function is used to asynchronously read a certain number of bytes of
 * data from a stream. The function call always returns immediately. The
 * asynchronous operation will continue until one of the following conditions is
 * true:
 *
 * @li The supplied buffers are full. That is, the bytes transferred is equal to
 * the sum of the buffer sizes.
 *
 * @li The completion_condition function object returns 0.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the AsyncReadStream concept.
 *
 * @param buffers One or more buffers into which the data will be read. The sum
 * of the buffer sizes indicates the maximum number of bytes to read from the
 * stream. Although the buffers object may be copied as necessary, ownership of
 * the underlying memory blocks is retained by the caller, which must guarantee
 * that they remain valid until the handler is called.
 *
 * @param completion_condition The function object to be called to determine
 * whether the read operation is complete. The signature of the function object
 * must be:
 * @code std::size_t completion_condition(
 *   // Result of latest async_read_some operation.
 *   const boost::system::error_code& error,
 *
 *   // Number of bytes transferred so far.
 *   std::size_t bytes_transferred
 * ); @endcode
 * A return value of 0 indicates that the read operation is complete. A non-zero
 * return value indicates the maximum number of bytes to be read on the next
 * call to the stream's async_read_some function.
 *
 * @param handler The handler to be called when the read operation completes.
 * Copies will be made of the handler as required. The function signature of the
 * handler must be:
 * @code void handler(
 *   const boost::system::error_code& error, // Result of operation.
 *
 *   std::size_t bytes_transferred           // Number of bytes copied into the
 *                                           // buffers. If an error occurred,
 *                                           // this will be the  number of
 *                                           // bytes successfully transferred
 *                                           // prior to the error.
 * ); @endcode
 * Regardless of whether the asynchronous operation completes immediately or
 * not, the handler will not be invoked from within this function. Invocation of
 * the handler will be performed in a manner equivalent to using
 * boost::asio::io_service::post().
 *
 * @par Example
 * To read into a single data buffer use the @ref buffer function as follows:
 * @code boost::asio::async_read(s,
 *     boost::asio::buffer(data, size),
 *     boost::asio::transfer_at_least(32),
 *     handler); @endcode
 * See the @ref buffer documentation for information on reading into multiple
 * buffers in one go, and how to use it with arrays, boost::array or
 * std::vector.
 */
template <typename AsyncReadStream, typename MutableBufferSequence,
    typename CompletionCondition, typename ReadHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler,
    void (boost::system::error_code, std::size_t))
async_read(AsyncReadStream& s, const MutableBufferSequence& buffers,
    CompletionCondition completion_condition,
    BOOST_ASIO_MOVE_ARG(ReadHandler) handler);

#if !defined(BOOST_ASIO_NO_IOSTREAM)

/// Start an asynchronous operation to read a certain amount of data from a
/// stream.
/**
 * This function is used to asynchronously read a certain number of bytes of
 * data from a stream. The function call always returns immediately. The
 * asynchronous operation will continue until one of the following conditions is
 * true:
 *
 * @li The supplied buffer is full (that is, it has reached maximum size).
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * async_read_some function, and is known as a <em>composed operation</em>. The
 * program must ensure that the stream performs no other read operations (such
 * as async_read, the stream's async_read_some function, or any other composed
 * operations that perform reads) until this operation completes.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the AsyncReadStream concept.
 *
 * @param b A basic_streambuf object into which the data will be read. Ownership
 * of the streambuf is retained by the caller, which must guarantee that it
 * remains valid until the handler is called.
 *
 * @param handler The handler to be called when the read operation completes.
 * Copies will be made of the handler as required. The function signature of the
 * handler must be:
 * @code void handler(
 *   const boost::system::error_code& error, // Result of operation.
 *
 *   std::size_t bytes_transferred           // Number of bytes copied into the
 *                                           // buffers. If an error occurred,
 *                                           // this will be the  number of
 *                                           // bytes successfully transferred
 *                                           // prior to the error.
 * ); @endcode
 * Regardless of whether the asynchronous operation completes immediately or
 * not, the handler will not be invoked from within this function. Invocation of
 * the handler will be performed in a manner equivalent to using
 * boost::asio::io_service::post().
 *
 * @note This overload is equivalent to calling:
 * @code boost::asio::async_read(
 *     s, b,
 *     boost::asio::transfer_all(),
 *     handler); @endcode
 */
template <typename AsyncReadStream, typename Allocator, typename ReadHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler,
    void (boost::system::error_code, std::size_t))
async_read(AsyncReadStream& s, basic_streambuf<Allocator>& b,
    BOOST_ASIO_MOVE_ARG(ReadHandler) handler);

/// Start an asynchronous operation to read a certain amount of data from a
/// stream.
/**
 * This function is used to asynchronously read a certain number of bytes of
 * data from a stream. The function call always returns immediately. The
 * asynchronous operation will continue until one of the following conditions is
 * true:
 *
 * @li The supplied buffer is full (that is, it has reached maximum size).
 *
 * @li The completion_condition function object returns 0.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * async_read_some function, and is known as a <em>composed operation</em>. The
 * program must ensure that the stream performs no other read operations (such
 * as async_read, the stream's async_read_some function, or any other composed
 * operations that perform reads) until this operation completes.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the AsyncReadStream concept.
 *
 * @param b A basic_streambuf object into which the data will be read. Ownership
 * of the streambuf is retained by the caller, which must guarantee that it
 * remains valid until the handler is called.
 *
 * @param completion_condition The function object to be called to determine
 * whether the read operation is complete. The signature of the function object
 * must be:
 * @code std::size_t completion_condition(
 *   // Result of latest async_read_some operation.
 *   const boost::system::error_code& error,
 *
 *   // Number of bytes transferred so far.
 *   std::size_t bytes_transferred
 * ); @endcode
 * A return value of 0 indicates that the read operation is complete. A non-zero
 * return value indicates the maximum number of bytes to be read on the next
 * call to the stream's async_read_some function.
 *
 * @param handler The handler to be called when the read operation completes.
 * Copies will be made of the handler as required. The function signature of the
 * handler must be:
 * @code void handler(
 *   const boost::system::error_code& error, // Result of operation.
 *
 *   std::size_t bytes_transferred           // Number of bytes copied into the
 *                                           // buffers. If an error occurred,
 *                                           // this will be the  number of
 *                                           // bytes successfully transferred
 *                                           // prior to the error.
 * ); @endcode
 * Regardless of whether the asynchronous operation completes immediately or
 * not, the handler will not be invoked from within this function. Invocation of
 * the handler will be performed in a manner equivalent to using
 * boost::asio::io_service::post().
 */
template <typename AsyncReadStream, typename Allocator,
    typename CompletionCondition, typename ReadHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler,
    void (boost::system::error_code, std::size_t))
async_read(AsyncReadStream& s, basic_streambuf<Allocator>& b,
    CompletionCondition completion_condition,
    BOOST_ASIO_MOVE_ARG(ReadHandler) handler);

#endif // !defined(BOOST_ASIO_NO_IOSTREAM)

/*@}*/

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#include <boost/asio/impl/read.hpp>

#endif // BOOST_ASIO_READ_HPP
