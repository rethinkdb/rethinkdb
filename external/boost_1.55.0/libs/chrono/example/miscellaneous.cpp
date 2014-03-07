//  miscellaneous.cpp  ----------------------------------------------------------//

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

// miscellaneous tests and demos:

#include <cassert>
#include <iostream>

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

int main()
{
    using namespace boost;
    drive_physics_function();
    test_range();
    test_extended_range();
    inspect_all();
    test_milliseconds();
    inspect_duration(common_type<duration<double>, hours, microseconds>::type(),
                    "common_type<duration<double>, hours, microseconds>::type");
    duration<double, boost::milli> d = milliseconds(3) * 2.5;
    inspect_duration(milliseconds(3) * 2.5, "milliseconds(3) * 2.5");
    std::cout << d.count() << '\n';
//    milliseconds ms(3.5);  // doesn't compile
//    std::cout << "milliseconds ms(3.5) doesn't compile\n";
    return 0;
}

