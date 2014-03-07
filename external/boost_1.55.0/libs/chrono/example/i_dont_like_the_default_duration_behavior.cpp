//  i_dont_like_the_default_duration_behavior.cpp  ----------------------------------------------------------//

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

#include <boost/chrono/chrono.hpp>
#include <boost/type_traits.hpp>

#include <iostream>

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
typedef boost::chrono::duration<zero_default<long long>                      > seconds;
typedef boost::chrono::duration<zero_default<long long>, boost::ratio<60>   > minutes;
typedef boost::chrono::duration<zero_default<long long>, boost::ratio<3600> > hours;

void test()
{
    milliseconds ms;
    std::cout << ms.count() << '\n';
}

}  // I_dont_like_the_default_duration_behavior


int main()
{
    I_dont_like_the_default_duration_behavior::test();
    return 0;
}

