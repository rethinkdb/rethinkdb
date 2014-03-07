// password.cpp
//
// Copyright (c) 2010
// Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[password
/*`
    For the source of this example see
    [@boost://libs/random/example/password.cpp password.cpp].

    This example demonstrates generating a random 8 character
    password.
 */


#include <boost/random/random_device.hpp>
#include <boost/random/uniform_int_distribution.hpp>

int main() {
    /*<< We first define the characters that we're going
         to allow.  This is pretty much just the characters
         on a standard keyboard.
    >>*/
    std::string chars(
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "1234567890"
        "!@#$%^&*()"
        "`~-_=+[{]{\\|;:'\",<.>/? ");
    /*<< We use __random_device as a source of entropy, since we want
         passwords that are not predictable.
    >>*/
    boost::random::random_device rng;
    /*<< Finally we select 8 random characters from the
         string and print them to cout.
    >>*/
    boost::random::uniform_int_distribution<> index_dist(0, chars.size() - 1);
    for(int i = 0; i < 8; ++i) {
        std::cout << chars[index_dist(rng)];
    }
    std::cout << std::endl;
}

//]
