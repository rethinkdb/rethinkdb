//
// waitable_timer_service.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_WAITABLE_TIMER_SERVICE_HPP
#define BOOST_ASIO_WAITABLE_TIMER_SERVICE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <cstddef>
#include <boost/asio/async_result.hpp>
#include <boost/asio/detail/chrono_time_traits.hpp>
#include <boost/asio/detail/deadline_timer_service.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/wait_traits.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

/// Default service implementation for a timer.
template <typename Clock,
    typename WaitTraits = boost::asio::wait_traits<Clock> >
class waitable_timer_service
#if defined(GENERATING_DOCUMENTATION)
  : public boost::asio::io_service::service
#else
  : public boost::asio::detail::service_base<
      waitable_timer_service<Clock, WaitTraits> >
#endif
{
public:
#if defined(GENERATING_DOCUMENTATION)
  /// The unique service identifier.
  static boost::asio::io_service::id id;
#endif

  /// The clock type.
  typedef Clock clock_type;

  /// The duration type of the clock.
  typedef typename clock_type::duration duration;

  /// The time point type of the clock.
  typedef typename clock_type::time_point time_point;

  /// The wait traits type.
  typedef WaitTraits traits_type;

private:
  // The type of the platform-specific implementation.
  typedef detail::deadline_timer_service<
    detail::chrono_time_traits<Clock, WaitTraits> > service_impl_type;

public:
  /// The implementation type of the waitable timer.
#if defined(GENERATING_DOCUMENTATION)
  typedef implementation_defined implementation_type;
#else
  typedef typename service_impl_type::implementation_type implementation_type;
#endif

  /// Construct a new timer service for the specified io_service.
  explicit waitable_timer_service(boost::asio::io_service& io_service)
    : boost::asio::detail::service_base<
        waitable_timer_service<Clock, WaitTraits> >(io_service),
      service_impl_(io_service)
  {
  }

  /// Construct a new timer implementation.
  void construct(implementation_type& impl)
  {
    service_impl_.construct(impl);
  }

  /// Destroy a timer implementation.
  void destroy(implementation_type& impl)
  {
    service_impl_.destroy(impl);
  }

  /// Cancel any asynchronous wait operations associated with the timer.
  std::size_t cancel(implementation_type& impl, boost::system::error_code& ec)
  {
    return service_impl_.cancel(impl, ec);
  }

  /// Cancels one asynchronous wait operation associated with the timer.
  std::size_t cancel_one(implementation_type& impl,
      boost::system::error_code& ec)
  {
    return service_impl_.cancel_one(impl, ec);
  }

  /// Get the expiry time for the timer as an absolute time.
  time_point expires_at(const implementation_type& impl) const
  {
    return service_impl_.expires_at(impl);
  }

  /// Set the expiry time for the timer as an absolute time.
  std::size_t expires_at(implementation_type& impl,
      const time_point& expiry_time, boost::system::error_code& ec)
  {
    return service_impl_.expires_at(impl, expiry_time, ec);
  }

  /// Get the expiry time for the timer relative to now.
  duration expires_from_now(const implementation_type& impl) const
  {
    return service_impl_.expires_from_now(impl);
  }

  /// Set the expiry time for the timer relative to now.
  std::size_t expires_from_now(implementation_type& impl,
      const duration& expiry_time, boost::system::error_code& ec)
  {
    return service_impl_.expires_from_now(impl, expiry_time, ec);
  }

  // Perform a blocking wait on the timer.
  void wait(implementation_type& impl, boost::system::error_code& ec)
  {
    service_impl_.wait(impl, ec);
  }

  // Start an asynchronous wait on the timer.
  template <typename WaitHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(WaitHandler,
      void (boost::system::error_code))
  async_wait(implementation_type& impl,
      BOOST_ASIO_MOVE_ARG(WaitHandler) handler)
  {
    detail::async_result_init<
      WaitHandler, void (boost::system::error_code)> init(
        BOOST_ASIO_MOVE_CAST(WaitHandler)(handler));

    service_impl_.async_wait(impl, init.handler);

    return init.result.get();
  }

private:
  // Destroy all user-defined handler objects owned by the service.
  void shutdown_service()
  {
    service_impl_.shutdown_service();
  }

  // The platform-specific implementation.
  service_impl_type service_impl_;
};

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_WAITABLE_TIMER_SERVICE_HPP
