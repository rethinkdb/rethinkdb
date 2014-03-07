/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(STOP_WATCH_HPP_HK040911_INCLUDED)
#define STOP_WATCH_HPP_HK040911_INCLUDED

#include <boost/config.hpp>
#include <boost/timer.hpp>

///////////////////////////////////////////////////////////////////////////////
//  
class stop_watch : public boost::timer {

    typedef boost::timer base_t;
    
public:
    stop_watch() : is_suspended_since(0), suspended_overall(0) {}

    void suspend()
    {
        if (0 == is_suspended_since) {
        // if not already suspended
            is_suspended_since = this->base_t::elapsed();
        }
    }
    void resume()
    {
        if (0 != is_suspended_since) {
        // if really suspended
            suspended_overall += this->base_t::elapsed() - is_suspended_since;
            is_suspended_since = 0;
        }
    }
    double elapsed() const
    {
        if (0 == is_suspended_since) {
        // currently running
            return this->base_t::elapsed() - suspended_overall;
        }

    // currently suspended
        BOOST_ASSERT(is_suspended_since >= suspended_overall);
        return is_suspended_since - suspended_overall;
    }
    
    std::string format_elapsed_time() const
    {
    double current = elapsed();
    char time_buffer[sizeof("1234:56:78.90 abcd.")+1];

        using namespace std;
        if (current >= 3600) {
        // show hours
            sprintf (time_buffer, "%d:%02d:%02d.%03d hrs.",
                (int)(current) / 3600, ((int)(current) % 3600) / 60,
                ((int)(current) % 3600) % 60, 
                (int)(current * 1000) % 1000);
        }
        else if (current >= 60) {
        // show minutes
            sprintf (time_buffer, "%d:%02d.%03d min.", 
                (int)(current) / 60, (int)(current) % 60, 
                (int)(current * 1000) % 1000);
        }
        else {
        // show seconds
            sprintf(time_buffer, "%d.%03d sec.", (int)current, 
                (int)(current * 1000) % 1000);
        }
        return time_buffer;
    }
    
private:
    double is_suspended_since;
    double suspended_overall; 
};

#endif // !defined(STOP_WATCH_HPP_HK040911_INCLUDED)
