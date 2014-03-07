//  errored_clock.hpp  --------------------------------------------------------------//

//  Copyright 2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_CHRONO_ERRORED_CLOCKS_HPP
#define BOOST_CHRONO_ERRORED_CLOCKS_HPP

#include <boost/chrono/config.hpp>
#include <boost/chrono/duration.hpp>
#include <boost/chrono/time_point.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>
#include <boost/chrono/detail/system.hpp>

  class errored_clock
  {
  public:
      typedef boost::chrono::nanoseconds                          duration;
      typedef duration::rep                        rep;
      typedef duration::period                     period;
      typedef boost::chrono::time_point<errored_clock>  time_point;
      static const bool is_steady =             true;
      static int errno_;

      static void set_errno(int err) {
          errno_=err;
      }

      // throws on error
      static time_point  now() {
          boost::throw_exception(
                  boost::system::system_error(
                          errno_,
                          BOOST_CHRONO_SYSTEM_CATEGORY,
                          "errored_clock"
                  )
          );
          return time_point();
      }
      // never throws and set ec
      static time_point  now(boost::system::error_code & ec) {
          if (BOOST_CHRONO_IS_THROWS(ec))
          {
              boost::throw_exception(
                      boost::system::system_error(
                              errno_,
                              BOOST_CHRONO_SYSTEM_CATEGORY,
                              "errored_clock"
                      )
              );
          }
          ec.assign( errno_, BOOST_CHRONO_SYSTEM_CATEGORY );
          return time_point();
      };
  };
  int errored_clock::errno_;

#endif
