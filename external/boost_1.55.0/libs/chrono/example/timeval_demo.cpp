//  timeval_demo.cpp  ----------------------------------------------------------//

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
#endif

#if defined(BOOST_CHRONO_WINDOWS_API)
#    include <windows.h>
#endif

#if defined(BOOST_CHRONO_WINDOWS_API)

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

//  timeval clock demo
//     Demonstrate the use of a timeval-like struct to be used as the representation
//     type for both duraiton and time_point.

namespace timeval_demo
{

class xtime {
private:
    long tv_sec;
    long tv_usec;

    void fixup() {
        if (tv_usec < 0) {
            tv_usec += 1000000;
            --tv_sec;
        }
    }

public:

    explicit xtime(long sec, long usec) {
        tv_sec = sec;
        tv_usec = usec;
        if (tv_usec < 0 || tv_usec >= 1000000) {
            tv_sec += tv_usec / 1000000;
            tv_usec %= 1000000;
            fixup();
        }
    }

    explicit xtime(long long usec)
    {
        tv_usec = static_cast<long>(usec % 1000000);
        tv_sec  = static_cast<long>(usec / 1000000);
        fixup();
    }

    // explicit
    operator long long() const {return static_cast<long long>(tv_sec) * 1000000 + tv_usec;}

    xtime& operator += (xtime rhs) {
        tv_sec += rhs.tv_sec;
        tv_usec += rhs.tv_usec;
        if (tv_usec >= 1000000) {
            tv_usec -= 1000000;
            ++tv_sec;
        }
        return *this;
    }

    xtime& operator -= (xtime rhs) {
        tv_sec -= rhs.tv_sec;
        tv_usec -= rhs.tv_usec;
        fixup();
        return *this;
    }

    xtime& operator %= (xtime rhs) {
        long long t = tv_sec * 1000000 + tv_usec;
        long long r = rhs.tv_sec * 1000000 + rhs.tv_usec;
        t %= r;
        tv_sec = static_cast<long>(t / 1000000);
        tv_usec = static_cast<long>(t % 1000000);
        fixup();
        return *this;
    }

    friend xtime operator+(xtime x, xtime y) {return x += y;}
    friend xtime operator-(xtime x, xtime y) {return x -= y;}
    friend xtime operator%(xtime x, xtime y) {return x %= y;}

    friend bool operator==(xtime x, xtime y)
        { return (x.tv_sec == y.tv_sec && x.tv_usec == y.tv_usec); }

    friend bool operator<(xtime x, xtime y) {
        if (x.tv_sec == y.tv_sec)
            return (x.tv_usec < y.tv_usec);
        return (x.tv_sec < y.tv_sec);
    }

    friend bool operator!=(xtime x, xtime y) { return !(x == y); }
    friend bool operator> (xtime x, xtime y) { return y < x; }
    friend bool operator<=(xtime x, xtime y) { return !(y < x); }
    friend bool operator>=(xtime x, xtime y) { return !(x < y); }

    friend std::ostream& operator<<(std::ostream& os, xtime x)
        {return os << '{' << x.tv_sec << ',' << x.tv_usec << '}';}
};

class xtime_clock
{
public:
    typedef xtime                                  rep;
    typedef boost::micro                           period;
    typedef boost::chrono::duration<rep, period>   duration;
    typedef boost::chrono::time_point<xtime_clock> time_point;

    static time_point now();
};


xtime_clock::time_point
xtime_clock::now()
{
#if defined(BOOST_CHRONO_WINDOWS_API)
    timeval tv;
    gettimeofday(&tv, 0);
    xtime xt( tv.tv_sec, tv.tv_usec);
    return time_point(duration(xt));

#elif defined(BOOST_CHRONO_MAC_API)

    timeval tv;
    gettimeofday(&tv, 0);
    xtime xt( tv.tv_sec, tv.tv_usec);
    return time_point(duration(xt));

#elif defined(BOOST_CHRONO_POSIX_API)
    //time_point t(0,0);

    timespec ts;
    ::clock_gettime( CLOCK_REALTIME, &ts );

    xtime xt( ts.tv_sec, ts.tv_nsec/1000);
    return time_point(duration(xt));
#endif  // POSIX

}

void test_xtime_clock()
{
    using namespace boost::chrono;
    std::cout << "timeval_demo system clock test\n";
    std::cout << "sizeof xtime_clock::time_point = " << sizeof(xtime_clock::time_point) << '\n';
    std::cout << "sizeof xtime_clock::duration = " << sizeof(xtime_clock::duration) << '\n';
    std::cout << "sizeof xtime_clock::rep = " << sizeof(xtime_clock::rep) << '\n';
    xtime_clock::duration delay(milliseconds(5));
    xtime_clock::time_point start = xtime_clock::now();

    while (xtime_clock::now() - start <= delay)
    {
    }
    xtime_clock::time_point stop = xtime_clock::now();
    xtime_clock::duration elapsed = stop - start;
    std::cout << "paused " << nanoseconds(elapsed).count() << " nanoseconds\n";
}

}  // timeval_demo

int main()
{
    timeval_demo::test_xtime_clock();
    return 0;
}

