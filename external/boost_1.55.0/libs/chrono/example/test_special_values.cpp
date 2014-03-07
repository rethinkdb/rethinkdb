//  test_special_values.cpp  ----------------------------------------------------------//

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

using namespace boost::chrono;

void test_special_values()
{
    std::cout << "duration<unsigned>::min().count()  = " << ((duration<unsigned>::min)()).count() << '\n';
    std::cout << "duration<unsigned>::zero().count() = " << duration<unsigned>::zero().count() << '\n';
    std::cout << "duration<unsigned>::max().count()  = " << ((duration<unsigned>::max)()).count() << '\n';
    std::cout << "duration<int>::min().count()       = " << ((duration<int>::min)()).count() << '\n';
    std::cout << "duration<int>::zero().count()      = " << duration<int>::zero().count() << '\n';
    std::cout << "duration<int>::max().count()       = " << ((duration<int>::max)()).count() << '\n';
}

int main()
{
    test_special_values();
    return 0;
}

