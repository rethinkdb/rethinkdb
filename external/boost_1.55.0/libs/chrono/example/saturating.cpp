//  saturating.cpp  ----------------------------------------------------------//

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

//////////////////////////////////////////////////////////
//////////////////// User2 Example ///////////////////////
//////////////////////////////////////////////////////////

// Demonstrate User2:
// A "saturating" signed integral type  is developed.  This type has +/- infinity and a nan
// (like IEEE floating point) but otherwise obeys signed integral arithmetic.
// This class is subsequently used as the rep in boost::chrono::duration to demonstrate a
// duration class that does not silently ignore overflow.
#include <ostream>
#include <stdexcept>
#include <climits>

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
saturate<I>::operator I() const
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
    static Rep min BOOST_PREVENT_MACRO_SUBSTITUTION ()  {return -(max)();}
};

}  // namespace chrono

}  // namespace boost

#include <iostream>

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


int main()
{
    testStdUser();
    testUser2();
    return 0;
}

