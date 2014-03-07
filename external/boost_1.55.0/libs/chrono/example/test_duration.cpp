//  test_duration.cpp  ----------------------------------------------------------//

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

#include <boost/assert.hpp>
#include <boost/chrono/chrono.hpp>
#include <boost/type_traits.hpp>

#include <iostream>

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



using namespace boost::chrono;
void test_duration_division()
{
    typedef boost::common_type<boost::chrono::hours::rep, boost::chrono::minutes::rep>::type h_min_rep;
    h_min_rep r3 = hours(3) / minutes(5);
    std::cout << r3 << '\n';
    std::cout << hours(3) / minutes(5) << '\n';
    std::cout << hours(3) / milliseconds(5) << '\n';
    std::cout << milliseconds(5) / hours(3) << '\n';
    std::cout << hours(1) / milliseconds(1) << '\n';
}

void test_duration_multiply()
{
    hours h15= 5 * hours(3);
    hours h6= hours(3) *2;
}

void f(duration<double> d, double res)  // accept floating point seconds
{
    // d.count() == 3.e-6 when passed microseconds(3)
    BOOST_ASSERT(d.count()==res);
}

void g(nanoseconds d, boost::intmax_t res)
{
    // d.count() == 3000 when passed microseconds(3)
    std::cout << d.count() << " " <<res << std::endl;
    BOOST_ASSERT(d.count()==res);
}

template <class Rep, class Period>
void tmpl(duration<Rep, Period> d, boost::intmax_t res)
{
    // convert d to nanoseconds, rounding up if it is not an exact conversion
    nanoseconds ns = duration_cast<nanoseconds>(d);
    if (ns < d)
        ++ns;
    // ns.count() == 333333334 when passed 1/3 of a floating point second
    BOOST_ASSERT(ns.count()==res);
}

template <class Period>
void tmpl2(duration<long long, Period> d, boost::intmax_t res)
{
    // convert d to nanoseconds, rounding up if it is not an exact conversion
    nanoseconds ns = duration_cast<nanoseconds>(d);
    if (ns < d)
        ++ns;
    // ns.count() == 333333334 when passed 333333333333 picoseconds
    BOOST_ASSERT(ns.count()==res);
}



int main()
{
    minutes m1(3);                 // m1 stores 3
    minutes m2(2);                 // m2 stores 2
    minutes m3 = m1 + m2;          // m3 stores 5
    BOOST_ASSERT(m3.count()==5);

    microseconds us1(3);           // us1 stores 3
    microseconds us2(2);           // us2 stores 2
    microseconds us3 = us1 + us2;  // us3 stores 5
    BOOST_ASSERT(us3.count()==5);

    microseconds us4 = m3 + us3;   // us4 stores 300000005
    BOOST_ASSERT(us4.count()==300000005);
    microseconds us5 = m3;   // us4 stores 300000000
    BOOST_ASSERT(us5.count()==300000000);

    //minutes m4 = m3 + us3; // won't compile

    minutes m4 = duration_cast<minutes>(m3 + us3);  // m4.count() == 5
    BOOST_ASSERT(m4.count()==5);

    typedef duration<double, boost::ratio<60> > dminutes;
    dminutes dm4 = m3 + us3;  // dm4.count() == 5.000000083333333
    BOOST_ASSERT(dm4.count()==5.000000083333333);

    f(microseconds(3), 0.000003);
    g(microseconds(3), 3000);
    duration<double> s(1./3);  // 1/3 of a second
    g(duration_cast<nanoseconds>(s), 333333333);  // round towards zero in conversion to nanoseconds
    //f(s);  // does not compile
    tmpl(duration<double>(1./3), 333333334);
    tmpl2(duration<long long, boost::pico>(333333333333LL), 333333334);  // About 1/3 of a second worth of picoseconds

    //f(3,3);  // Will not compile, 3 is not implicitly convertible to any `duration`
    //g(3,3);  // Will not compile, 3 is not implicitly convertible to any `duration`
    //tmpl(3,3);  // Will not compile, 3 is not implicitly convertible to any `duration`
    //tmpl2(3,3);  // Will not compile, 3 is not implicitly convertible to any `duration`

    {
    double r = double(milliseconds(3) / milliseconds(3));
    std::cout << r << '\n';

    duration<double, boost::milli> d = milliseconds(3) * 2.5;
    duration<double, boost::milli> d2 = 2.5 * milliseconds(3) ;
    duration<double, boost::milli> d3 = milliseconds(3) / 2.5;
    duration<double, boost::milli> d4 = milliseconds(3) + milliseconds(5) ;
    inspect_duration(milliseconds(3) * 2.5, "milliseconds(3) * 2.5");
    std::cout << d.count() << '\n';
//    milliseconds ms(3.5);  // doesn't compile
    std::cout << "milliseconds ms(3.5) doesn't compile\n";
    }

    test_duration_division();
    test_duration_multiply();
    return 0;
}

