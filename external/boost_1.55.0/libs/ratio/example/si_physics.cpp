//  ratio_test.cpp  ----------------------------------------------------------//

//  Copyright 2008 Howard Hinnant
//  Copyright 2008 Beman Dawes

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

#include <iostream>
#include <boost/ratio/ratio.hpp>
#include "duration.hpp"

namespace User1
{
// Example type-safe "physics" code interoperating with chrono::duration types
//  and taking advantage of the std::ratio infrastructure and design philosophy.

// length - mimics chrono::duration except restricts representation to double.
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
typedef boost_ex::chrono::duration<double> seconds;                         // unity
// Demo of (scientific) support for sub-nanosecond resolutions
typedef boost_ex::chrono::duration<double,  boost::pico> picosecond;  // 10^-12 seconds
typedef boost_ex::chrono::duration<double, boost::femto> femtosecond; // 10^-15 seconds
typedef boost_ex::chrono::duration<double,  boost::atto> attosecond;  // 10^-18 seconds

// A very brief proof-of-concept for SIUnits-like library
//  Hard-wired to floating point seconds and meters, but accepts other units (shown in testUser1())
template <class R1, class R2>
class quantity
{
    double q_;
public:
    typedef R1 time_dim;
    typedef R2 distance_dim;
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



int main()
{
    //~ typedef boost::ratio<8, BOOST_INTMAX_C(0x7FFFFFFFD)> R1;
    //~ typedef boost::ratio<3, BOOST_INTMAX_C(0x7FFFFFFFD)> R2;
    typedef User1::quantity<boost::ratio_subtract<boost::ratio<0>, boost::ratio<1> >::type,
                             boost::ratio_subtract<boost::ratio<1>, boost::ratio<0> >::type > RR;
    //~ typedef boost::ratio_subtract<R1, R2>::type RS;
    //~ std::cout << RS::num << '/' << RS::den << '\n';


    std::cout << "*************\n";
    std::cout << "* testUser1 *\n";
    std::cout << "*************\n";
    User1::Distance d(( User1::mile(110) ));
    boost_ex::chrono::hours h((2));
    User1::Time t(( h ));
    //~ boost_ex::chrono::seconds sss=boost_ex::chrono::duration_cast<boost_ex::chrono::seconds>(h);
    //~ User1::seconds sss((120));
    //~ User1::Time t(( sss ));

    //typedef User1::quantity<boost::ratio_subtract<User1::Distance::time_dim, User1::Time::time_dim >::type,
    //                        boost::ratio_subtract<User1::Distance::distance_dim, User1::Time::distance_dim >::type > R;
    RR r=d / t;
    //r.set(d.get() / t.get());

    User1::Speed rc= r;
    (void)rc;
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
  return 0;
}
