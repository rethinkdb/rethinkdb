//  time2_demo.cpp  ----------------------------------------------------------//

//  Copyright 2008 Howard Hinnant
//  Copyright 2008 Beman Dawes

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

/*

This code was derived by Beman Dawes from Howard Hinnant's time2_demo prototype.
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

#include <cassert>
#include <climits>
#include <iostream>
#include <ostream>
#include <stdexcept>

#include <windows.h>

namespace
{
  //struct timeval {
  //        long    tv_sec;         /* seconds */
  //        long    tv_usec;        /* and microseconds */
  //};

  int gettimeofday(struct timeval * tp, void *)
  {
    FILETIME ft;
    ::GetSystemTimeAsFileTime( &ft );  // never fails
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

//////////////////////////////////////////////////////////
///////////// simulated thread interface /////////////////
//////////////////////////////////////////////////////////


namespace std {

void __print_time(boost::chrono::system_clock::time_point t)
{
    using namespace boost::chrono;
    time_t c_time = system_clock::to_time_t(t);
    std::tm* tmptr = std::localtime(&c_time);
    system_clock::duration d = t.time_since_epoch();
    std::cout << tmptr->tm_hour << ':' << tmptr->tm_min << ':' << tmptr->tm_sec
              << '.' << (d - duration_cast<seconds>(d)).count();
}

namespace this_thread {

template <class Rep, class Period>
void sleep_for(const boost::chrono::duration<Rep, Period>& d)
{
    boost::chrono::microseconds t = boost::chrono::duration_cast<boost::chrono::microseconds>(d);
    if (t < d)
        ++t;
    if (t > boost::chrono::microseconds(0))
        std::cout << "sleep_for " << t.count() << " microseconds\n";
}

template <class Clock, class Duration>
void sleep_until(const boost::chrono::time_point<Clock, Duration>& t)
{
    using namespace boost::chrono;
    typedef time_point<Clock, Duration> Time;
    typedef system_clock::time_point SysTime;
    if (t > Clock::now())
    {
        typedef typename boost::common_type<typename Time::duration,
                                     typename SysTime::duration>::type D;
        /* auto */ D d = t - Clock::now();
        microseconds us = duration_cast<microseconds>(d);
        if (us < d)
            ++us;
        SysTime st = system_clock::now() + us;
        std::cout << "sleep_until    ";
        __print_time(st);
        std::cout << " which is " << (st - system_clock::now()).count() << " microseconds away\n";
    }
}

}  // this_thread

struct mutex {};

struct timed_mutex
{
    bool try_lock() {std::cout << "timed_mutex::try_lock()\n"; return true;}

    template <class Rep, class Period>
        bool try_lock_for(const boost::chrono::duration<Rep, Period>& d)
        {
            boost::chrono::microseconds t = boost::chrono::duration_cast<boost::chrono::microseconds>(d);
            if (t <= boost::chrono::microseconds(0))
                return try_lock();
            std::cout << "try_lock_for " << t.count() << " microseconds\n";
            return true;
        }

    template <class Clock, class Duration>
    bool try_lock_until(const boost::chrono::time_point<Clock, Duration>& t)
    {
        using namespace boost::chrono;
        typedef time_point<Clock, Duration> Time;
        typedef system_clock::time_point SysTime;
        if (t <= Clock::now())
            return try_lock();
        typedef typename boost::common_type<typename Time::duration,
          typename Clock::duration>::type D;
        /* auto */ D d = t - Clock::now();
        microseconds us = duration_cast<microseconds>(d);
        SysTime st = system_clock::now() + us;
        std::cout << "try_lock_until ";
        __print_time(st);
        std::cout << " which is " << (st - system_clock::now()).count()
          << " microseconds away\n";
        return true;
    }
};

struct condition_variable
{
    template <class Rep, class Period>
        bool wait_for(mutex&, const boost::chrono::duration<Rep, Period>& d)
        {
            boost::chrono::microseconds t = boost::chrono::duration_cast<boost::chrono::microseconds>(d);
            std::cout << "wait_for " << t.count() << " microseconds\n";
            return true;
        }

    template <class Clock, class Duration>
    bool wait_until(mutex&, const boost::chrono::time_point<Clock, Duration>& t)
    {
        using namespace boost::chrono;
        typedef time_point<Clock, Duration> Time;
        typedef system_clock::time_point SysTime;
        if (t <= Clock::now())
            return false;
        typedef typename boost::common_type<typename Time::duration,
          typename Clock::duration>::type D;
        /* auto */ D d = t - Clock::now();
        microseconds us = duration_cast<microseconds>(d);
        SysTime st = system_clock::now() + us;
         std::cout << "wait_until     ";
        __print_time(st);
        std::cout << " which is " << (st - system_clock::now()).count()
          << " microseconds away\n";
        return true;
    }
};

} // namespace std

//////////////////////////////////////////////////////////
//////////// Simple sleep and wait examples //////////////
//////////////////////////////////////////////////////////

std::mutex m;
std::timed_mutex mut;
std::condition_variable cv;

void basic_examples()
{
    std::cout << "Running basic examples\n";
    using namespace std;
    using namespace boost::chrono;
    system_clock::time_point time_limit = system_clock::now() + seconds(4) + milliseconds(500);
    this_thread::sleep_for(seconds(3));
    this_thread::sleep_for(nanoseconds(300));
    this_thread::sleep_until(time_limit);
//    this_thread::sleep_for(time_limit);  // desired compile-time error
//    this_thread::sleep_until(seconds(3)); // desired compile-time error
    mut.try_lock_for(milliseconds(30));
    mut.try_lock_until(time_limit);
//    mut.try_lock_for(time_limit);        // desired compile-time error
//    mut.try_lock_until(milliseconds(30)); // desired compile-time error
    cv.wait_for(m, minutes(1));    // real code would put this in a loop
    cv.wait_until(m, time_limit);  // real code would put this in a loop
    // For those who prefer floating point
    this_thread::sleep_for(duration<double>(0.25));
    this_thread::sleep_until(system_clock::now() + duration<double>(1.5));
}

//////////////////////////////////////////////////////////
//////////////////// User1 Example ///////////////////////
//////////////////////////////////////////////////////////

namespace User1
{
// Example type-safe "physics" code interoperating with boost::chrono::duration types
//  and taking advantage of the boost::ratio infrastructure and design philosophy.

// length - mimics boost::chrono::duration except restricts representation to double.
//    Uses boost::ratio facilities for length units conversions.

template <class Ratio>
class length
{
public:
    typedef Ratio ratio;
private:
    double len_;
public:

    length() : len_(1) {}
    length(const double& len) : len_(len) {}

    // conversions
    template <class R>
    length(const length<R>& d)
            : len_(d.count() * boost::ratio_divide<Ratio, R>::type::den /
                               boost::ratio_divide<Ratio, R>::type::num) {}

    // observer

    double count() const {return len_;}

    // arithmetic

    length& operator+=(const length& d) {len_ += d.count(); return *this;}
    length& operator-=(const length& d) {len_ -= d.count(); return *this;}

    length operator+() const {return *this;}
    length operator-() const {return length(-len_);}

    length& operator*=(double rhs) {len_ *= rhs; return *this;}
    length& operator/=(double rhs) {len_ /= rhs; return *this;}
};

// Sparse sampling of length units
typedef length<boost::ratio<1> >          meter;        // set meter as "unity"
typedef length<boost::centi>              centimeter;   // 1/100 meter
typedef length<boost::kilo>               kilometer;    // 1000  meters
typedef length<boost::ratio<254, 10000> > inch;         // 254/10000 meters
// length takes ratio instead of two integral types so that definitions can be made like so:
typedef length<boost::ratio_multiply<boost::ratio<12>, inch::ratio>::type>   foot;  // 12 inchs
typedef length<boost::ratio_multiply<boost::ratio<5280>, foot::ratio>::type> mile;  // 5280 feet

// Need a floating point definition of seconds
typedef boost::chrono::duration<double> seconds;                         // unity
// Demo of (scientific) support for sub-nanosecond resolutions
typedef boost::chrono::duration<double,  boost::pico> picosecond;  // 10^-12 seconds
typedef boost::chrono::duration<double, boost::femto> femtosecond; // 10^-15 seconds
typedef boost::chrono::duration<double,  boost::atto> attosecond;  // 10^-18 seconds

// A very brief proof-of-concept for SIUnits-like library
//  Hard-wired to floating point seconds and meters, but accepts other units (shown in testUser1())
template <class R1, class R2>
class quantity
{
    double q_;
public:
    quantity() : q_(1) {}

    double get() const {return q_;}
    void set(double q) {q_ = q;}
};

template <>
class quantity<boost::ratio<1>, boost::ratio<0> >
{
    double q_;
public:
    quantity() : q_(1) {}
    quantity(seconds d) : q_(d.count()) {}  // note:  only User1::seconds needed here

    double get() const {return q_;}
    void set(double q) {q_ = q;}
};

template <>
class quantity<boost::ratio<0>, boost::ratio<1> >
{
    double q_;
public:
    quantity() : q_(1) {}
    quantity(meter d) : q_(d.count()) {}  // note:  only User1::meter needed here

    double get() const {return q_;}
    void set(double q) {q_ = q;}
};

template <>
class quantity<boost::ratio<0>, boost::ratio<0> >
{
    double q_;
public:
    quantity() : q_(1) {}
    quantity(double d) : q_(d) {}

    double get() const {return q_;}
    void set(double q) {q_ = q;}
};

// Example SI-Units
typedef quantity<boost::ratio<0>, boost::ratio<0> >  Scalar;
typedef quantity<boost::ratio<1>, boost::ratio<0> >  Time;         // second
typedef quantity<boost::ratio<0>, boost::ratio<1> >  Distance;     // meter
typedef quantity<boost::ratio<-1>, boost::ratio<1> > Speed;        // meter/second
typedef quantity<boost::ratio<-2>, boost::ratio<1> > Acceleration; // meter/second^2

template <class R1, class R2, class R3, class R4>
quantity<typename boost::ratio_subtract<R1, R3>::type, typename boost::ratio_subtract<R2, R4>::type>
operator/(const quantity<R1, R2>& x, const quantity<R3, R4>& y)
{
    typedef quantity<typename boost::ratio_subtract<R1, R3>::type, typename boost::ratio_subtract<R2, R4>::type> R;
    R r;
    r.set(x.get() / y.get());
    return r;
}

template <class R1, class R2, class R3, class R4>
quantity<typename boost::ratio_add<R1, R3>::type, typename boost::ratio_add<R2, R4>::type>
operator*(const quantity<R1, R2>& x, const quantity<R3, R4>& y)
{
    typedef quantity<typename boost::ratio_add<R1, R3>::type, typename boost::ratio_add<R2, R4>::type> R;
    R r;
    r.set(x.get() * y.get());
    return r;
}

template <class R1, class R2>
quantity<R1, R2>
operator+(const quantity<R1, R2>& x, const quantity<R1, R2>& y)
{
    typedef quantity<R1, R2> R;
    R r;
    r.set(x.get() + y.get());
    return r;
}

template <class R1, class R2>
quantity<R1, R2>
operator-(const quantity<R1, R2>& x, const quantity<R1, R2>& y)
{
    typedef quantity<R1, R2> R;
    R r;
    r.set(x.get() - y.get());
    return r;
}

// Example type-safe physics function
Distance
compute_distance(Speed v0, Time t, Acceleration a)
{
    return v0 * t + Scalar(.5) * a * t * t;  // if a units mistake is made here it won't compile
}

} // User1


// Exercise example type-safe physics function and show interoperation
// of custom time durations (User1::seconds) and standard time durations (std::hours).
// Though input can be arbitrary (but type-safe) units, output is always in SI-units
//   (a limitation of the simplified Units lib demoed here).
void testUser1()
{
    std::cout << "*************\n";
    std::cout << "* testUser1 *\n";
    std::cout << "*************\n";
    User1::Distance d( User1::mile(110) );
    User1::Time t( boost::chrono::hours(2) );
    User1::Speed s = d / t;
    std::cout << "Speed = " << s.get() << " meters/sec\n";
    User1::Acceleration a = User1::Distance( User1::foot(32.2) ) / User1::Time() / User1::Time();
    std::cout << "Acceleration = " << a.get() << " meters/sec^2\n";
    User1::Distance df = compute_distance(s, User1::Time( User1::seconds(0.5) ), a);
    std::cout << "Distance = " << df.get() << " meters\n";
    std::cout << "There are " << User1::mile::ratio::den << '/' << User1::mile::ratio::num << " miles/meter";
    User1::meter mt = 1;
    User1::mile mi = mt;
    std::cout << " which is approximately " << mi.count() << '\n';
    std::cout << "There are " << User1::mile::ratio::num << '/' << User1::mile::ratio::den << " meters/mile";
    mi = 1;
    mt = mi;
    std::cout << " which is approximately " << mt.count() << '\n';
    User1::attosecond as(1);
    User1::seconds sec = as;
    std::cout << "1 attosecond is " << sec.count() << " seconds\n";
    std::cout << "sec = as;  // compiles\n";
    sec = User1::seconds(1);
    as = sec;
    std::cout << "1 second is " << as.count() << " attoseconds\n";
    std::cout << "as = sec;  // compiles\n";
    std::cout << "\n";
}

//////////////////////////////////////////////////////////
//////////////////// User2 Example ///////////////////////
//////////////////////////////////////////////////////////

// Demonstrate User2:
// A "saturating" signed integral type  is developed.  This type has +/- infinity and a nan
// (like IEEE floating point) but otherwise obeys signed integral arithmetic.
// This class is subsequently used as the rep in boost::chrono::duration to demonstrate a
// duration class that does not silently ignore overflow.

namespace User2
{

template <class I>
class saturate
{
public:
    typedef I int_type;

    static const int_type nan = int_type(int_type(1) << (sizeof(int_type) * CHAR_BIT - 1));
    static const int_type neg_inf = nan + 1;
    static const int_type pos_inf = -neg_inf;
private:
    int_type i_;

//     static_assert(std::is_integral<int_type>::value && std::is_signed<int_type>::value,
//                   "saturate only accepts signed integral types");
//     static_assert(nan == -nan && neg_inf < pos_inf,
//                   "saturate assumes two's complement hardware for signed integrals");

public:
    saturate() : i_(nan) {}
    explicit saturate(int_type i) : i_(i) {}
    // explicit
       operator int_type() const;

    saturate& operator+=(saturate x);
    saturate& operator-=(saturate x) {return *this += -x;}
    saturate& operator*=(saturate x);
    saturate& operator/=(saturate x);
    saturate& operator%=(saturate x);

    saturate  operator- () const {return saturate(-i_);}
    saturate& operator++()       {*this += saturate(int_type(1)); return *this;}
    saturate  operator++(int)    {saturate tmp(*this); ++(*this); return tmp;}
    saturate& operator--()       {*this -= saturate(int_type(1)); return *this;}
    saturate  operator--(int)    {saturate tmp(*this); --(*this); return tmp;}

    friend saturate operator+(saturate x, saturate y) {return x += y;}
    friend saturate operator-(saturate x, saturate y) {return x -= y;}
    friend saturate operator*(saturate x, saturate y) {return x *= y;}
    friend saturate operator/(saturate x, saturate y) {return x /= y;}
    friend saturate operator%(saturate x, saturate y) {return x %= y;}

    friend bool operator==(saturate x, saturate y)
    {
        if (x.i_ == nan || y.i_ == nan)
            return false;
        return x.i_ == y.i_;
    }

    friend bool operator!=(saturate x, saturate y) {return !(x == y);}

    friend bool operator<(saturate x, saturate y)
    {
        if (x.i_ == nan || y.i_ == nan)
            return false;
        return x.i_ < y.i_;
    }

    friend bool operator<=(saturate x, saturate y)
    {
        if (x.i_ == nan || y.i_ == nan)
            return false;
        return x.i_ <= y.i_;
    }

    friend bool operator>(saturate x, saturate y)
    {
        if (x.i_ == nan || y.i_ == nan)
            return false;
        return x.i_ > y.i_;
    }

    friend bool operator>=(saturate x, saturate y)
    {
        if (x.i_ == nan || y.i_ == nan)
            return false;
        return x.i_ >= y.i_;
    }

    friend std::ostream& operator<<(std::ostream& os, saturate s)
    {
        switch (s.i_)
        {
        case pos_inf:
            return os << "inf";
        case nan:
            return os << "nan";
        case neg_inf:
            return os << "-inf";
        };
        return os << s.i_;
    }
};

template <class I>
saturate<I>::operator int_type() const
{
    switch (i_)
    {
    case nan:
    case neg_inf:
    case pos_inf:
        throw std::out_of_range("saturate special value can not convert to int_type");
    }
    return i_;
}

template <class I>
saturate<I>&
saturate<I>::operator+=(saturate x)
{
    switch (i_)
    {
    case pos_inf:
        switch (x.i_)
        {
        case neg_inf:
        case nan:
            i_ = nan;
        }
        return *this;
    case nan:
        return *this;
    case neg_inf:
        switch (x.i_)
        {
        case pos_inf:
        case nan:
            i_ = nan;
        }
        return *this;
    }
    switch (x.i_)
    {
    case pos_inf:
    case neg_inf:
    case nan:
        i_ = x.i_;
        return *this;
    }
    if (x.i_ >= 0)
    {
        if (i_ < pos_inf - x.i_)
            i_ += x.i_;
        else
            i_ = pos_inf;
        return *this;
    }
    if (i_ > neg_inf - x.i_)
        i_ += x.i_;
    else
        i_ = neg_inf;
    return *this;
}

template <class I>
saturate<I>&
saturate<I>::operator*=(saturate x)
{
    switch (i_)
    {
    case 0:
        switch (x.i_)
        {
        case pos_inf:
        case neg_inf:
        case nan:
            i_ = nan;
        }
        return *this;
    case pos_inf:
        switch (x.i_)
        {
        case nan:
        case 0:
            i_ = nan;
            return *this;
        }
        if (x.i_ < 0)
            i_ = neg_inf;
        return *this;
    case nan:
        return *this;
    case neg_inf:
        switch (x.i_)
        {
        case nan:
        case 0:
            i_ = nan;
            return *this;
        }
        if (x.i_ < 0)
            i_ = pos_inf;
        return *this;
    }
    switch (x.i_)
    {
    case 0:
        i_ = 0;
        return *this;
    case nan:
        i_ = nan;
        return *this;
    case pos_inf:
        if (i_ < 0)
            i_ = neg_inf;
        else
            i_ = pos_inf;
        return *this;
    case neg_inf:
        if (i_ < 0)
            i_ = pos_inf;
        else
            i_ = neg_inf;
        return *this;
    }
    int s = (i_ < 0 ? -1 : 1) * (x.i_ < 0 ? -1 : 1);
    i_ = i_ < 0 ? -i_ : i_;
    int_type x_i_ = x.i_ < 0 ? -x.i_ : x.i_;
    if (i_ <= pos_inf / x_i_)
        i_ *= x_i_;
    else
        i_ = pos_inf;
    i_ *= s;
    return *this;
}

template <class I>
saturate<I>&
saturate<I>::operator/=(saturate x)
{
    switch (x.i_)
    {
    case pos_inf:
    case neg_inf:
        switch (i_)
        {
        case pos_inf:
        case neg_inf:
        case nan:
            i_ = nan;
            break;
        default:
            i_ = 0;
            break;
        }
        return *this;
    case nan:
        i_ = nan;
        return *this;
    case 0:
        switch (i_)
        {
        case pos_inf:
        case neg_inf:
        case nan:
            return *this;
        case 0:
            i_ = nan;
            return *this;
        }
        if (i_ > 0)
            i_ = pos_inf;
        else
            i_ = neg_inf;
        return *this;
    }
    switch (i_)
    {
    case 0:
    case nan:
        return *this;
    case pos_inf:
    case neg_inf:
        if (x.i_ < 0)
            i_ = -i_;
        return *this;
    }
    i_ /= x.i_;
    return *this;
}

template <class I>
saturate<I>&
saturate<I>::operator%=(saturate x)
{
//    *this -= *this / x * x;  // definition
    switch (x.i_)
    {
    case nan:
    case neg_inf:
    case 0:
    case pos_inf:
        i_ = nan;
        return *this;
    }
    switch (i_)
    {
    case neg_inf:
    case pos_inf:
        i_ = nan;
    case nan:
        return *this;
    }
    i_ %= x.i_;
    return *this;
}

// Demo overflow-safe integral durations ranging from picoseconds resolution to millennium resolution
typedef boost::chrono::duration<saturate<long long>, boost::pico                 > picoseconds;
typedef boost::chrono::duration<saturate<long long>, boost::nano                 > nanoseconds;
typedef boost::chrono::duration<saturate<long long>, boost::micro                > microseconds;
typedef boost::chrono::duration<saturate<long long>, boost::milli                > milliseconds;
typedef boost::chrono::duration<saturate<long long>                            > seconds;
typedef boost::chrono::duration<saturate<long long>, boost::ratio<         60LL> > minutes;
typedef boost::chrono::duration<saturate<long long>, boost::ratio<       3600LL> > hours;
typedef boost::chrono::duration<saturate<long long>, boost::ratio<      86400LL> > days;
typedef boost::chrono::duration<saturate<long long>, boost::ratio<   31556952LL> > years;
typedef boost::chrono::duration<saturate<long long>, boost::ratio<31556952000LL> > millennium;

}  // User2

// Demonstrate custom promotion rules (needed only if there are no implicit conversions)
namespace User2 { namespace detail {

template <class T1, class T2, bool = boost::is_integral<T1>::value>
struct promote_helper;

template <class T1, class T2>
struct promote_helper<T1, saturate<T2>, true>  // integral
{
    typedef typename boost::common_type<T1, T2>::type rep;
    typedef User2::saturate<rep> type;
};

template <class T1, class T2>
struct promote_helper<T1, saturate<T2>, false>  // floating
{
    typedef T1 type;
};

} }

namespace boost
{

template <class T1, class T2>
struct common_type<User2::saturate<T1>, User2::saturate<T2> >
{
    typedef typename common_type<T1, T2>::type rep;
    typedef User2::saturate<rep> type;
};

template <class T1, class T2>
struct common_type<T1, User2::saturate<T2> >
    : User2::detail::promote_helper<T1, User2::saturate<T2> > {};

template <class T1, class T2>
struct common_type<User2::saturate<T1>, T2>
    : User2::detail::promote_helper<T2, User2::saturate<T1> > {};


// Demonstrate specialization of duration_values:

namespace chrono {

template <class I>
struct duration_values<User2::saturate<I> >
{
    typedef User2::saturate<I> Rep;
public:
    static Rep zero() {return Rep(0);}
    static Rep max BOOST_PREVENT_MACRO_SUBSTITUTION ()  {return Rep(Rep::pos_inf-1);}
    static Rep min BOOST_PREVENT_MACRO_SUBSTITUTION ()  {return -(max) ();}
};

}  // namespace chrono

}  // namespace boost


void testUser2()
{
    std::cout << "*************\n";
    std::cout << "* testUser2 *\n";
    std::cout << "*************\n";
    using namespace User2;
    typedef seconds::rep sat;
    years yr(sat(100));
    std::cout << "100 years expressed as years = " << yr.count() << '\n';
    nanoseconds ns = yr;
    std::cout << "100 years expressed as nanoseconds = " << ns.count() << '\n';
    ns += yr;
    std::cout << "200 years expressed as nanoseconds = " << ns.count() << '\n';
    ns += yr;
    std::cout << "300 years expressed as nanoseconds = " << ns.count() << '\n';
//    yr = ns;  // does not compile
    std::cout << "yr = ns;  // does not compile\n";
//    picoseconds ps1 = yr;  // does not compile, compile-time overflow in ratio arithmetic
    std::cout << "ps = yr;  // does not compile\n";
    ns = yr;
    picoseconds ps = ns;
    std::cout << "100 years expressed as picoseconds = " << ps.count() << '\n';
    ps = ns / sat(1000);
    std::cout << "0.1 years expressed as picoseconds = " << ps.count() << '\n';
    yr = years(sat(-200000000));
    std::cout << "200 million years ago encoded in years: " << yr.count() << '\n';
    days d = boost::chrono::duration_cast<days>(yr);
    std::cout << "200 million years ago encoded in days: " << d.count() << '\n';
    millennium c = boost::chrono::duration_cast<millennium>(yr);
    std::cout << "200 million years ago encoded in millennium: " << c.count() << '\n';
    std::cout << "Demonstrate \"uninitialized protection\" behavior:\n";
    seconds sec;
    for (++sec; sec < seconds(sat(10)); ++sec)
        ;
    std::cout << sec.count() << '\n';
    std::cout << "\n";
}

void testStdUser()
{
    std::cout << "***************\n";
    std::cout << "* testStdUser *\n";
    std::cout << "***************\n";
    using namespace boost::chrono;
    hours hr = hours(100);
    std::cout << "100 hours expressed as hours = " << hr.count() << '\n';
    nanoseconds ns = hr;
    std::cout << "100 hours expressed as nanoseconds = " << ns.count() << '\n';
    ns += hr;
    std::cout << "200 hours expressed as nanoseconds = " << ns.count() << '\n';
    ns += hr;
    std::cout << "300 hours expressed as nanoseconds = " << ns.count() << '\n';
//    hr = ns;  // does not compile
    std::cout << "hr = ns;  // does not compile\n";
//    hr * ns;  // does not compile
    std::cout << "hr * ns;  // does not compile\n";
    duration<double> fs(2.5);
    std::cout << "duration<double> has count() = " << fs.count() << '\n';
//    seconds sec = fs;  // does not compile
    std::cout << "seconds sec = duration<double> won't compile\n";
    seconds sec = duration_cast<seconds>(fs);
    std::cout << "seconds has count() = " << sec.count() << '\n';
    std::cout << "\n";
}

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
    time_point t(duration(xtime(0)));
    gettimeofday((timeval*)&t, 0);
    return t;
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

// Handle duration with resolution not known until run time

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
       operator tonanosec() const {return tonanosec(rep_/ticks_per_nanosecond);}

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
    //mach_timebase_info_data_t MachInfo;
    //mach_timebase_info(&MachInfo);
    //return static_cast<double>(MachInfo.denom) / MachInfo.numer;
    return static_cast<double>(1) / 1000; // Windows FILETIME is 1 per microsec
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
    typedef duration::rep rep;
    typedef runtime_resolution::duration duration;
    typedef runtime_resolution::time_point time_point;

    static time_point now()
    {
      timeval tv;
      gettimeofday( &tv, 0 );
      return time_point(duration((static_cast<rep>(tv.tv_sec)<<32) | tv.tv_usec));
    }
};

void test()
{
    using namespace boost::chrono;
    std::cout << "runtime_resolution test\n";
    clock::duration delay(boost::chrono::milliseconds(5));
    clock::time_point start = clock::now();
    while (clock::now() - start <= delay)
      ;
    clock::time_point stop = clock::now();
    clock::duration elapsed = stop - start;
    std::cout << "paused " << nanoseconds(duration_cast<nanoseconds>(duration::tonanosec(elapsed))).count()
                           << " nanoseconds\n";
}

}  // runtime_resolution

// miscellaneous tests and demos:


using namespace boost::chrono;

void physics_function(duration<double> d)
{
    std::cout << "d = " << d.count() << '\n';
}

void drive_physics_function()
{
    physics_function(nanoseconds(3));
    physics_function(hours(3));
    physics_function(duration<double>(2./3));
    std::cout.precision(16);
    physics_function( hours(3) + nanoseconds(-3) );
}

void test_range()
{
    using namespace boost::chrono;
    hours h1 = hours(24 * ( 365 * 292 + 292/4));
    nanoseconds n1 = h1 + nanoseconds(1);
    nanoseconds delta = n1 - h1;
    std::cout << "292 years of hours = " << h1.count() << "hr\n";
    std::cout << "Add a nanosecond = " << n1.count() << "ns\n";
    std::cout << "Find the difference = " << delta.count() << "ns\n";
}

void test_extended_range()
{
    using namespace boost::chrono;
    hours h1 = hours(24 * ( 365 * 244000 + 244000/4));
    /*auto*/ microseconds u1 = h1 + microseconds(1);
    /*auto*/ microseconds delta = u1 - h1;
    std::cout << "244,000 years of hours = " << h1.count() << "hr\n";
    std::cout << "Add a microsecond = " << u1.count() << "us\n";
    std::cout << "Find the difference = " << delta.count() << "us\n";
}

template <class Rep, class Period>
void inspect_duration(boost::chrono::duration<Rep, Period> d, const std::string& name)
{
    typedef boost::chrono::duration<Rep, Period> Duration;
    std::cout << "********* " << name << " *********\n";
    std::cout << "The period of " << name << " is " << (double)Period::num/Period::den << " seconds.\n";
    std::cout << "The frequency of " << name << " is " << (double)Period::den/Period::num << " Hz.\n";
    std::cout << "The representation is ";
    if (boost::is_floating_point<Rep>::value)
    {
        std::cout << "floating point\n";
        std::cout << "The precision is the most significant ";
        std::cout << std::numeric_limits<Rep>::digits10 << " decimal digits.\n";
    }
    else if (boost::is_integral<Rep>::value)
    {
        std::cout << "integral\n";
        d = Duration(Rep(1));
        boost::chrono::duration<double> dsec = d;
        std::cout << "The precision is " << dsec.count() << " seconds.\n";
    }
    else
    {
        std::cout << "a class type\n";
        d = Duration(Rep(1));
        boost::chrono::duration<double> dsec = d;
        std::cout << "The precision is " << dsec.count() << " seconds.\n";
    }
    d = Duration((std::numeric_limits<Rep>::max)());
    using namespace boost::chrono;
    using namespace std;
    typedef duration<double, boost::ratio_multiply<boost::ratio<24*3652425,10000>, hours::period>::type> Years;
    Years years = d;
    std::cout << "The range is +/- " << years.count() << " years.\n";
    std::cout << "sizeof(" << name << ") = " << sizeof(d) << '\n';
}

void inspect_all()
{
    using namespace boost::chrono;
    std::cout.precision(6);
    inspect_duration(nanoseconds(), "nanoseconds");
    inspect_duration(microseconds(), "microseconds");
    inspect_duration(milliseconds(), "milliseconds");
    inspect_duration(seconds(), "seconds");
    inspect_duration(minutes(), "minutes");
    inspect_duration(hours(), "hours");
    inspect_duration(duration<double>(), "duration<double>");
}

void test_milliseconds()
{
    using namespace boost::chrono;
    milliseconds ms(250);
    ms += milliseconds(1);
    milliseconds ms2(150);
    milliseconds msdiff = ms - ms2;
    if (msdiff == milliseconds(101))
        std::cout << "success\n";
    else
        std::cout << "failure: " << msdiff.count() << '\n';
}

    using namespace std;
    using namespace boost::chrono;

// Example round_up utility:  converts d to To, rounding up for inexact conversions
//   Being able to *easily* write this function is a major feature!
template <class To, class Rep, class Period>
To
round_up(duration<Rep, Period> d)
{
    To result = duration_cast<To>(d);
    if (result < d)
        ++result;
    return result;
}

// demonstrate interaction with xtime-like facility:

using namespace boost::chrono;

struct xtime
{
    long sec;
    unsigned long usec;
};

template <class Rep, class Period>
xtime
to_xtime_truncate(duration<Rep, Period> d)
{
    xtime xt;
    xt.sec = static_cast<long>(duration_cast<seconds>(d).count());
    xt.usec = static_cast<long>(duration_cast<microseconds>(d - seconds(xt.sec)).count());
    return xt;
}

template <class Rep, class Period>
xtime
to_xtime_round_up(duration<Rep, Period> d)
{
    xtime xt;
    xt.sec = static_cast<long>(duration_cast<seconds>(d).count());
    xt.usec = static_cast<unsigned long>(round_up<microseconds>(d - seconds(xt.sec)).count());
    return xt;
}

microseconds
from_xtime(xtime xt)
{
    return seconds(xt.sec) + microseconds(xt.usec);
}

void print(xtime xt)
{
    cout << '{' << xt.sec << ',' << xt.usec << "}\n";
}

void test_with_xtime()
{
    cout << "test_with_xtime\n";
    xtime xt = to_xtime_truncate(seconds(3) + milliseconds(251));
    print(xt);
    milliseconds ms = duration_cast<milliseconds>(from_xtime(xt));
    cout << ms.count() << " milliseconds\n";
    xt = to_xtime_round_up(ms);
    print(xt);
    xt = to_xtime_truncate(seconds(3) + nanoseconds(999));
    print(xt);
    xt = to_xtime_round_up(seconds(3) + nanoseconds(999));
    print(xt);
}

void test_system_clock()
{
    cout << "system_clock test" << endl;
    system_clock::duration delay = milliseconds(5);
    system_clock::time_point start = system_clock::now();
    while (system_clock::now() - start <= delay)
        ;
    system_clock::time_point stop = system_clock::now();
    system_clock::duration elapsed = stop - start;
    cout << "paused " << nanoseconds(elapsed).count() << " nanoseconds\n";
    start = system_clock::now();
    stop = system_clock::now();
    cout << "system_clock resolution estimate: " << nanoseconds(stop-start).count() << " nanoseconds\n";
}

void test_steady_clock()
{
    cout << "steady_clock test" << endl;
    steady_clock::duration delay = milliseconds(5);
    steady_clock::time_point start = steady_clock::now();
    while (steady_clock::now() - start <= delay)
        ;
    steady_clock::time_point stop = steady_clock::now();
    steady_clock::duration elapsed = stop - start;
    cout << "paused " << nanoseconds(elapsed).count() << " nanoseconds\n";
    start = steady_clock::now();
    stop = steady_clock::now();
    cout << "steady_clock resolution estimate: " << nanoseconds(stop-start).count() << " nanoseconds\n";
}

void test_hi_resolution_clock()
{
    cout << "high_resolution_clock test" << endl;
    high_resolution_clock::duration delay = milliseconds(5);
    high_resolution_clock::time_point start = high_resolution_clock::now();
    while (high_resolution_clock::now() - start <= delay)
      ;
    high_resolution_clock::time_point stop = high_resolution_clock::now();
    high_resolution_clock::duration elapsed = stop - start;
    cout << "paused " << nanoseconds(elapsed).count() << " nanoseconds\n";
    start = high_resolution_clock::now();
    stop = high_resolution_clock::now();
    cout << "high_resolution_clock resolution estimate: " << nanoseconds(stop-start).count() << " nanoseconds\n";
}

//void test_mixed_clock()
//{
//    cout << "mixed clock test" << endl;
//    high_resolution_clock::time_point hstart = high_resolution_clock::now();
//    cout << "Add 5 milliseconds to a high_resolution_clock::time_point\n";
//    steady_clock::time_point mend = hstart + milliseconds(5);
//    bool b = hstart == mend;
//    system_clock::time_point sstart = system_clock::now();
//    std::cout << "Subtracting system_clock::time_point from steady_clock::time_point doesn't compile\n";
////  mend - sstart; // doesn't compile
//    cout << "subtract high_resolution_clock::time_point from steady_clock::time_point"
//            " and add that to a system_clock::time_point\n";
//    system_clock::time_point send = sstart + duration_cast<system_clock::duration>(mend - hstart);
//    cout << "subtract two system_clock::time_point's and output that in microseconds:\n";
//    microseconds ms = send - sstart;
//    cout << ms.count() << " microseconds\n";
//}
//
//void test_c_mapping()
//{
//    cout << "C map test\n";
//    using namespace boost::chrono;
//    system_clock::time_point t1 = system_clock::now();
//    std::time_t c_time = system_clock::to_time_t(t1);
//    std::tm* tmptr = std::localtime(&c_time);
//    std::cout << "It is now " << tmptr->tm_hour << ':' << tmptr->tm_min << ':' << tmptr->tm_sec << ' '
//              << tmptr->tm_year + 1900 << '-' << tmptr->tm_mon + 1 << '-' << tmptr->tm_mday << '\n';
//    c_time = std::mktime(tmptr);
//    system_clock::time_point t2 = system_clock::from_time_t(c_time);
//    microseconds ms = t1 - t2;
//    std::cout << "Round-tripping through the C interface truncated the precision by " << ms.count() << " microseconds\n";
//}

void test_duration_division()
{
    cout << hours(3) / milliseconds(5) << '\n';
    cout << milliseconds(5) / hours(3) << '\n';
    cout << hours(1) / milliseconds(1) << '\n';
}

namespace I_dont_like_the_default_duration_behavior
{

// Here's how you override the duration's default constructor to do anything you want (in this case zero)

template <class R>
class zero_default
{
public:
    typedef R rep;

private:
    rep rep_;
public:
    zero_default(rep i = 0) : rep_(i) {}
    operator rep() const {return rep_;}

    zero_default& operator+=(zero_default x) {rep_ += x.rep_; return *this;}
    zero_default& operator-=(zero_default x) {rep_ -= x.rep_; return *this;}
    zero_default& operator*=(zero_default x) {rep_ *= x.rep_; return *this;}
    zero_default& operator/=(zero_default x) {rep_ /= x.rep_; return *this;}

    zero_default  operator+ () const {return *this;}
    zero_default  operator- () const {return zero_default(-rep_);}
    zero_default& operator++()       {++rep_; return *this;}
    zero_default  operator++(int)    {return zero_default(rep_++);}
    zero_default& operator--()       {--rep_; return *this;}
    zero_default  operator--(int)    {return zero_default(rep_--);}

    friend zero_default operator+(zero_default x, zero_default y) {return x += y;}
    friend zero_default operator-(zero_default x, zero_default y) {return x -= y;}
    friend zero_default operator*(zero_default x, zero_default y) {return x *= y;}
    friend zero_default operator/(zero_default x, zero_default y) {return x /= y;}

    friend bool operator==(zero_default x, zero_default y) {return x.rep_ == y.rep_;}
    friend bool operator!=(zero_default x, zero_default y) {return !(x == y);}
    friend bool operator< (zero_default x, zero_default y) {return x.rep_ < y.rep_;}
    friend bool operator<=(zero_default x, zero_default y) {return !(y < x);}
    friend bool operator> (zero_default x, zero_default y) {return y < x;}
    friend bool operator>=(zero_default x, zero_default y) {return !(x < y);}
};

typedef boost::chrono::duration<zero_default<long long>, boost::nano        > nanoseconds;
typedef boost::chrono::duration<zero_default<long long>, boost::micro       > microseconds;
typedef boost::chrono::duration<zero_default<long long>, boost::milli       > milliseconds;
typedef boost::chrono::duration<zero_default<long long>                   > seconds;
typedef boost::chrono::duration<zero_default<long long>, boost::ratio<60>   > minutes;
typedef boost::chrono::duration<zero_default<long long>, boost::ratio<3600> > hours;

void test()
{
    milliseconds ms;
    cout << ms.count() << '\n';
}

}  // I_dont_like_the_default_duration_behavior

// Build a min for two time_points

template <class Rep, class Period>
void
print_duration(ostream& os, duration<Rep, Period> d)
{
    os << d.count() << " * " << Period::num << '/' << Period::den << " seconds\n";
}

// Example min utility:  returns the earliest time_point
//   Being able to *easily* write this function is a major feature!
template <class Clock, class Duration1, class Duration2>
inline
typename boost::common_type<time_point<Clock, Duration1>,
                     time_point<Clock, Duration2> >::type
min BOOST_PREVENT_MACRO_SUBSTITUTION (time_point<Clock, Duration1> t1, time_point<Clock, Duration2> t2)
{
    return t2 < t1 ? t2 : t1;
}

void test_min()
{
    typedef time_point<system_clock,
      boost::common_type<system_clock::duration, seconds>::type> T1;
    typedef time_point<system_clock,
      boost::common_type<system_clock::duration, nanoseconds>::type> T2;
    typedef boost::common_type<T1, T2>::type T3;
    /*auto*/ T1 t1 = system_clock::now() + seconds(3);
    /*auto*/ T2 t2 = system_clock::now() + nanoseconds(3);
    /*auto*/ T3 t3 = (min)(t1, t2);
    print_duration(cout, t1 - t3);
    print_duration(cout, t2 - t3);
}

void explore_limits()
{
    typedef duration<long long, boost::ratio_multiply<boost::ratio<24*3652425,10000>,
      hours::period>::type> Years;
    steady_clock::time_point t1( Years(250));
    steady_clock::time_point t2(-Years(250));
    // nanosecond resolution is likely to overflow.  "up cast" to microseconds.
    //   The "up cast" trades precision for range.
    microseconds d = time_point_cast<microseconds>(t1) - time_point_cast<microseconds>(t2);
    cout << d.count() << " microseconds\n";
}

void manipulate_clock_object(system_clock clock)
{
    system_clock::duration delay = milliseconds(5);
    system_clock::time_point start = clock.now();
    while (clock.now() - start <= delay)
      ;
    system_clock::time_point stop = clock.now();
    system_clock::duration elapsed = stop - start;
    cout << "paused " << nanoseconds(elapsed).count() << " nanoseconds\n";
};

template <long long speed>
struct cycle_count
{
    typedef typename boost::ratio_multiply<boost::ratio<speed>, boost::mega>::type frequency;  // Mhz
    typedef typename boost::ratio_divide<boost::ratio<1>, frequency>::type period;
    typedef long long rep;
    typedef boost::chrono::duration<rep, period> duration;
    typedef boost::chrono::time_point<cycle_count> time_point;

    static time_point now()
    {
        static long long tick = 0;
        // return exact cycle count
        return time_point(duration(++tick));  // fake access to clock cycle count
    }
};

template <long long speed>
struct approx_cycle_count
{
    static const long long frequency = speed * 1000000;  // MHz
    typedef nanoseconds duration;
    typedef duration::rep rep;
    typedef duration::period period;
    static const long long nanosec_per_sec = period::den;
    typedef boost::chrono::time_point<approx_cycle_count> time_point;

    static time_point now()
    {
        static long long tick = 0;
        // return cycle count as an approximate number of nanoseconds
        // compute as if nanoseconds is only duration in the std::lib
        return time_point(duration(++tick * nanosec_per_sec / frequency));
    }
};

void cycle_count_delay()
{
    {
    typedef cycle_count<400> clock;
    cout << "\nSimulated " << clock::frequency::num / boost::mega::num << "MHz clock which has a tick period of "
         << duration<double, boost::nano>(clock::duration(1)).count() << " nanoseconds\n";
    nanoseconds delayns(500);
    clock::duration delay = duration_cast<clock::duration>(delayns);
    cout << "delay = " << delayns.count() << " nanoseconds which is " << delay.count() << " cycles\n";
    clock::time_point start = clock::now();
    clock::time_point stop = start + delay;
    while (clock::now() < stop)  // no multiplies or divides in this loop
        ;
    clock::time_point end = clock::now();
    clock::duration elapsed = end - start;
    cout << "paused " << elapsed.count() << " cycles ";
    cout << "which is " << duration_cast<nanoseconds>(elapsed).count() << " nanoseconds\n";
    }
    {
    typedef approx_cycle_count<400> clock;
    cout << "\nSimulated " << clock::frequency / 1000000 << "MHz clock modeled with nanoseconds\n";
    clock::duration delay = nanoseconds(500);
    cout << "delay = " << delay.count() << " nanoseconds\n";
    clock::time_point start = clock::now();
    clock::time_point stop = start + delay;
    while (clock::now() < stop) // 1 multiplication and 1 division in this loop
        ;
    clock::time_point end = clock::now();
    clock::duration elapsed = end - start;
    cout << "paused " << elapsed.count() << " nanoseconds\n";
    }
    {
    typedef cycle_count<1500> clock;
    cout << "\nSimulated " << clock::frequency::num / boost::mega::num << "MHz clock which has a tick period of "
         << duration<double, boost::nano>(clock::duration(1)).count() << " nanoseconds\n";
    nanoseconds delayns(500);
    clock::duration delay = duration_cast<clock::duration>(delayns);
    cout << "delay = " << delayns.count() << " nanoseconds which is " << delay.count() << " cycles\n";
    clock::time_point start = clock::now();
    clock::time_point stop = start + delay;
    while (clock::now() < stop)  // no multiplies or divides in this loop
        ;
    clock::time_point end = clock::now();
    clock::duration elapsed = end - start;
    cout << "paused " << elapsed.count() << " cycles ";
    cout << "which is " << duration_cast<nanoseconds>(elapsed).count() << " nanoseconds\n";
    }
    {
    typedef approx_cycle_count<1500> clock;
    cout << "\nSimulated " << clock::frequency / 1000000 << "MHz clock modeled with nanoseconds\n";
    clock::duration delay = nanoseconds(500);
    cout << "delay = " << delay.count() << " nanoseconds\n";
    clock::time_point start = clock::now();
    clock::time_point stop = start + delay;
    while (clock::now() < stop) // 1 multiplication and 1 division in this loop
        ;
    clock::time_point end = clock::now();
    clock::duration elapsed = end - start;
    cout << "paused " << elapsed.count() << " nanoseconds\n";
    }
}

void test_special_values()
{
    std::cout << "duration<unsigned>::min().count()  = " << (duration<unsigned>::min)().count() << '\n';
    std::cout << "duration<unsigned>::zero().count() = " << duration<unsigned>::zero().count() << '\n';
    std::cout << "duration<unsigned>::max().count()  = " << (duration<unsigned>::max)().count() << '\n';
    std::cout << "duration<int>::min().count()       = " << (duration<int>::min)().count() << '\n';
    std::cout << "duration<int>::zero().count()      = " << duration<int>::zero().count() << '\n';
    std::cout << "duration<int>::max().count()       = " << (duration<int>::max)().count() << '\n';
}

int main()
{
    basic_examples();
    testStdUser();
    testUser1();
    testUser2();
    drive_physics_function();
    test_range();
    test_extended_range();
    inspect_all();
    test_milliseconds();
    test_with_xtime();
    test_system_clock();
    test_steady_clock();
    test_hi_resolution_clock();
    //test_mixed_clock();
    timeval_demo::test_xtime_clock();
    runtime_resolution::test();
    //test_c_mapping();
    test_duration_division();
    I_dont_like_the_default_duration_behavior::test();
    test_min();
    inspect_duration(common_type<duration<double>, hours, microseconds>::type(),
                    "common_type<duration<double>, hours, microseconds>::type");
    explore_limits();
    manipulate_clock_object(system_clock());
    duration<double, boost::milli> d = milliseconds(3) * 2.5;
    inspect_duration(milliseconds(3) * 2.5, "milliseconds(3) * 2.5");
    cout << d.count() << '\n';
//    milliseconds ms(3.5);  // doesn't compile
    cout << "milliseconds ms(3.5) doesn't compile\n";
    cycle_count_delay();
    test_special_values();
    return 0;
}

