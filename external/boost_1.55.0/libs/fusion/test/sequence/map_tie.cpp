/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>

#include <boost/fusion/container/generation/map_tie.hpp>
#include <boost/fusion/sequence/intrinsic/at_key.hpp>

struct key_zero;
struct key_one;

int main()
{
    using namespace boost::fusion;
    {
        int number = 101;
        char letter = 'a';
        BOOST_TEST(at_key<key_zero>(map_tie<key_zero, key_one>(number, letter)) == 101);
        BOOST_TEST(at_key<key_one>(map_tie<key_zero, key_one>(number, letter)) == 'a');

        BOOST_TEST(&at_key<key_zero>(map_tie<key_zero, key_one>(number, letter)) == &number);
        BOOST_TEST(&at_key<key_one>(map_tie<key_zero, key_one>(number, letter)) == &letter);

        at_key<key_zero>(map_tie<key_zero, key_one>(number, letter)) = 202;
        at_key<key_one>(map_tie<key_zero, key_one>(number, letter)) = 'b';

        BOOST_TEST(number == 202);
        BOOST_TEST(letter == 'b');
    }
    return boost::report_errors();
}
