//  runtime_resolution.cpp  ----------------------------------------------------------//

//  Copyright 2008 Howard Hinnant
//  Copyright 2008 Beman Dawes
//  Copyright 2009 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

/*
This code was extracted by Vicente J. Botet Escriba from Beman Dawes time2_demo.cpp which
was derived by Beman Dawes from Howard Hinnant's time2_demo prototype.
Many thanks to Howard for making his code available under the Boost license.
The original code was modified to conform to Boost conventions and to section
20.9 Time utilities [time] of the C++ committee's working paper N2798.
See http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2798.pdf.

time2_demo contained this comment:

    Much thanks to Andrei Alexandrescu,
                   Walter Brown,
                   Peter Dimov,
                   Jeff Garland,
                   Terry Golubiewski,
                   Daniel Krugler,
                   Anthony Williams.
*/
#define _CRT_SECURE_NO_WARNINGS  // disable VC++ foolishness

#include <boost/chrono/chrono.hpp>
#include <boost/type_traits.hpp>

#include <iostream>

#if defined(BOOST_CHRONO_MAC_API)
#include <sys/time.h> //for gettimeofday and timeval
#include <mach/mach_time.h>  // mach_absolute_time, mach_timebase_info_data_t
#endif

#if defined(BOOST_CHRONO_WINDOWS_API)
#include <windows.h>

namespace
{
  #ifdef UNDER_CE
  // Windows CE does not define timeval
  struct timeval {
          long    tv_sec;         /* seconds */
          long    tv_usec;        /* and microseconds */
  };
  #endif

  int gettimeofday(struct timeval * tp, void *)
  {
    FILETIME ft;
  #if defined(UNDER_CE)
    // Windows CE does not define GetSystemTimeAsFileTime so we do it in two steps.
    SYSTEMTIME st;
    ::GetSystemTime( &st );
    ::SystemTimeToFileTime( &st, &ft );
  #else
    ::GetSystemTimeAsFileTime( &ft );  // never fails
  #endif
    long long t = (static_cast<long long>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
  # if !defined( BOOST_MSVC ) || BOOST_MSVC > 1300 // > VC++ 7.0
    t -= 116444736000000000LL;
  # else
    t -= 116444736000000000;
  # endif
    t /= 10;  // microseconds
    tp->tv_sec = static_cast<long>( t / 1000000UL);
    tp->tv_usec = static_cast<long>( t % 1000000UL);
    return 0;
  }
}  // unnamed namespace

#endif

// Handle duration with resolution not known until run time
using namespace boost::chrono;

namespace runtime_resolution
{

class duration
{
public:
    typedef long long rep;
private:
    rep rep_;

    static const double ticks_per_nanosecond;

public:
    typedef boost::chrono::duration<double, boost::nano> tonanosec;

    duration() {} // = default;
    explicit duration(const rep& r) : rep_(r) {}

    // conversions
    explicit duration(const tonanosec& d)
            : rep_(static_cast<rep>(d.count() * ticks_per_nanosecond)) {}

    // explicit
    tonanosec convert_to_nanosec() const {return tonanosec(rep_/ticks_per_nanosecond);}

    // observer

    rep count() const {return rep_;}

    // arithmetic

    duration& operator+=(const duration& d) {rep_ += d.rep_; return *this;}
    duration& operator-=(const duration& d) {rep_ += d.rep_; return *this;}
    duration& operator*=(rep rhs)           {rep_ *= rhs; return *this;}
    duration& operator/=(rep rhs)           {rep_ /= rhs; return *this;}

    duration  operator+() const {return *this;}
    duration  operator-() const {return duration(-rep_);}
    duration& operator++()      {++rep_; return *this;}
    duration  operator++(int)   {return duration(rep_++);}
    duration& operator--()      {--rep_; return *this;}
    duration  operator--(int)   {return duration(rep_--);}

    friend duration operator+(duration x, duration y) {return x += y;}
    friend duration operator-(duration x, duration y) {return x -= y;}
    friend duration operator*(duration x, rep y)      {return x *= y;}
    friend duration operator*(rep x, duration y)      {return y *= x;}
    friend duration operator/(duration x, rep y)      {return x /= y;}

    friend bool operator==(duration x, duration y) {return x.rep_ == y.rep_;}
    friend bool operator!=(duration x, duration y) {return !(x == y);}
    friend bool operator< (duration x, duration y) {return x.rep_ < y.rep_;}
    friend bool operator<=(duration x, duration y) {return !(y < x);}
    friend bool operator> (duration x, duration y) {return y < x;}
    friend bool operator>=(duration x, duration y) {return !(x < y);}
};

static
double
init_duration()
{
#if defined(BOOST_CHRONO_WINDOWS_API)
    return static_cast<double>(1) / 1000; // Windows FILETIME is 1 per microsec
#elif defined(BOOST_CHRONO_MAC_API)
    mach_timebase_info_data_t MachInfo;
    mach_timebase_info(&MachInfo);
    return static_cast<double>(MachInfo.denom) / MachInfo.numer;
#elif defined(BOOST_CHRONO_POSIX_API)
    return static_cast<double>(1) / 1000;
#endif

}

const double duration::ticks_per_nanosecond = init_duration();

class clock;

class time_point
{
public:
    typedef runtime_resolution::clock clock;
    typedef long long rep;
private:
    rep rep_;


    rep count() const {return rep_;}
public:

    time_point() : rep_(0) {}
    explicit time_point(const duration& d)
        : rep_(d.count()) {}

    // arithmetic

    time_point& operator+=(const duration& d) {rep_ += d.count(); return *this;}
    time_point& operator-=(const duration& d) {rep_ -= d.count(); return *this;}

    friend time_point operator+(time_point x, duration y) {return x += y;}
    friend time_point operator+(duration x, time_point y) {return y += x;}
    friend time_point operator-(time_point x, duration y) {return x -= y;}
    friend duration operator-(time_point x, time_point y) {return duration(x.rep_ - y.rep_);}
};


class clock
{
public:
    typedef runtime_resolution::duration::rep rep;
    typedef runtime_resolution::duration duration;
    typedef runtime_resolution::time_point time_point;

    static time_point now()
    {

#if defined(BOOST_CHRONO_WINDOWS_API)
      timeval tv;
      gettimeofday( &tv, 0 );
      return time_point(duration((static_cast<rep>(tv.tv_sec)<<32) | tv.tv_usec));

#elif defined(BOOST_CHRONO_MAC_API)

      timeval tv;
      gettimeofday( &tv, 0 );
      return time_point(duration((static_cast<rep>(tv.tv_sec)<<32) | tv.tv_usec));

#elif defined(BOOST_CHRONO_POSIX_API)
    timespec ts;
    ::clock_gettime( CLOCK_REALTIME, &ts );

    return time_point(duration((static_cast<rep>(ts.tv_sec)<<32) | ts.tv_nsec/1000));


#endif  // POSIX

    }
};

void test()
{
    std::cout << "runtime_resolution test\n";
    clock::duration delay(boost::chrono::milliseconds(5));
    clock::time_point start = clock::now();
    while (clock::now() - start <= delay)
      ;
    clock::time_point stop = clock::now();
    clock::duration elapsed = stop - start;
    std::cout << "paused " << 
    boost::chrono::nanoseconds(
        boost::chrono::duration_cast<boost::chrono::nanoseconds>( elapsed.convert_to_nanosec() )).count()
                           << " nanoseconds\n";
}

}  // runtime_resolution


int main()
{
    runtime_resolution::test();
    return 0;
}

