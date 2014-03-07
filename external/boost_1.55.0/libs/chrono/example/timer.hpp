//  boost/chrono/timer.hpp  ------------------------------------------------------------//

//  Copyright Beman Dawes 2008
//  Copyright 2009 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/system for documentation.

#ifndef BOOSTEX_CHRONO_TIMER_HPP
#define BOOSTEX_CHRONO_TIMER_HPP

#include <boost/chrono/chrono.hpp>
#include <boost/system/error_code.hpp>

namespace boost_ex
{
  namespace chrono
  {

//--------------------------------------------------------------------------------------//
//                                    timer                                             //
//--------------------------------------------------------------------------------------//

    template <class Clock=boost::chrono::high_resolution_clock>
    class timer
    {
    public:
      typedef Clock                       clock;
      typedef typename Clock::duration    duration;
      typedef typename Clock::time_point  time_point;

      explicit timer( boost::system::error_code & ec = BOOST_CHRONO_THROWS )
        { 
          start(ec); 
          }

     ~timer() {}  // never throws

      void start( boost::system::error_code & ec = BOOST_CHRONO_THROWS )
        { 
          m_start = clock::now( ec ); 
          }

      duration elapsed( boost::system::error_code & ec = BOOST_CHRONO_THROWS )
        { return clock::now( ec ) - m_start; }

    private:
      time_point m_start;
    };

    typedef chrono::timer< boost::chrono::system_clock > system_timer;
#ifdef BOOST_CHRONO_HAS_CLOCK_STEADY
    typedef chrono::timer< boost::chrono::steady_clock > steady_timer;
#endif
    typedef chrono::timer< boost::chrono::high_resolution_clock > high_resolution_timer;

  } // namespace chrono
} // namespace boost_ex

#endif
