//  boost/chrono/round.hpp  ------------------------------------------------------------//

//  (C) Copyright Howard Hinnant
//  Copyright 2011 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/chrono for documentation.

#ifndef BOOST_CHRONO_FLOOR_HPP
#define BOOST_CHRONO_FLOOR_HPP

#include <boost/chrono/duration.hpp>

namespace boost
{
  namespace chrono
  {

    /**
     * rounds down
     */
    template <class To, class Rep, class Period>
    To floor(const duration<Rep, Period>& d)
    {
        return duration_cast<To>(d);
    }


  } // namespace chrono
} // namespace boost

#endif
