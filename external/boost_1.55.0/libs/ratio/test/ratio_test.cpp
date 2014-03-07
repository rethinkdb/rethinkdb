//  ratio_test.cpp  ----------------------------------------------------------//

//  Copyright 2008 Howard Hinnant
//  Copyright 2008 Beman Dawes

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

#include <boost/ratio/ratio.hpp>
#include <iostream>

typedef boost::ratio<5, 3>   five_thirds;       // five_thirds::num == 5, five_thirds::den == 3
typedef boost::ratio<25, 15> also_five_thirds;  // also_five_thirds::num == 5, also_five_thirds::den == 3
typedef boost::ratio_divide<five_thirds, also_five_thirds>::type one;  // one::num == 1, one::den == 1


typedef boost::ratio_multiply<boost::ratio<5>, boost::giga>::type _5giga;  // _5giga::num == 5000000000, _5giga::den == 1
typedef boost::ratio_multiply<boost::ratio<5>, boost::nano>::type _5nano;  // _5nano::num == 1, _5nano::den == 200000000

//  Test the case described in library working group issue 948.

typedef boost::ratio<BOOST_RATIO_INTMAX_T_MAX, BOOST_RATIO_INTMAX_T_MAX-16> R1;
typedef boost::ratio<8, 7> R2;
typedef boost::ratio_multiply<R1, R2>::type RT;



int main()
{
    typedef boost::ratio<8, BOOST_RATIO_INTMAX_C(0x7FFFFFFFD)> R1;
    typedef boost::ratio<3, BOOST_RATIO_INTMAX_C(0x7FFFFFFFD)> R2;
    typedef boost::ratio_subtract<R1, R2>::type RS;
    std::cout << RS::num << '/' << RS::den << '\n';

  return 0;
}
