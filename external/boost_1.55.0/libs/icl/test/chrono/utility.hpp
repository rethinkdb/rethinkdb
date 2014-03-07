/*-----------------------------------------------------------------------------+    
Copyright (c) 2011-2011: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_LIBS_ICL_TEST_CHRONO_UTILITY_HPP_JOFA_20110314
#define BOOST_LIBS_ICL_TEST_CHRONO_UTILITY_HPP_JOFA_20110314

#include <boost/chrono/chrono.hpp>
#include <boost/chrono/chrono_io.hpp>

// In order to perform simple testing with chrono::time_point
// we need to provide a dummy test clock

class Now // A trivial test clock
{         
public:
    typedef boost::chrono::duration<int>   duration;
    typedef duration::rep                  rep;
    typedef duration::period               periond;
    typedef boost::chrono::time_point<Now> time_point;

    static time_point now(){ return time_point(); }
};

namespace boost{ namespace chrono {

template <class CharT>
struct clock_string<Now, CharT>
{
    static std::basic_string<CharT> name()
    {
        static const CharT u[] = {'n', 'o', 'w', '_', 'c', 'l','o', 'c', 'k'};
        static const std::basic_string<CharT> str(u, u + sizeof(u)/sizeof(u[0]));
        return str;
    }
    static std::basic_string<CharT> since()
    {
        const CharT u[] = {' ', 's', 'i', 'n', 'c', 'e', ' ', 'n', 'o', 'w'};
        const std::basic_string<CharT> str(u, u + sizeof(u)/sizeof(u[0]));
        return str;
    }
};

}} //namespace boost chrono

#endif //BOOST_LIBS_ICL_TEST_CHRONO_UTILITY_HPP_JOFA_20110314
