/*-----------------------------------------------------------------------------+    
Author: Joachim Faulhaber
Copyright (c) 2009-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
/** Example large_bitset.cpp \file large_bitset.cpp
    \brief Shows a bitset class that combines interval and bitset compression
           in order to represent very large bitsets efficiently.

    Example large_bitset.cpp demonstrates the usage of a large_bitset class
    template that is implemented as an interval_map of bitsets. The idea is
    to combine interval compression and bitset compression. Large uninterrupted
    runs of bits are represented by intervals (interval compression). Local
    nests of varying bitsequences are represented by associated bitests
    (bitset compression).

    Find a commented sample implementation in the boost book documentation 
    <a href="http://www.joachim-faulhaber.de/boost_itl/doc/libs/icl/doc/html/boost_itl/projects.html#boost_itl.projects.large_bitset">
    here</a>.

    \include large_bitset_/large_bitset.cpp
*/
#if defined(_MSC_VER)
#pragma warning(disable:4244) // Msvc warns on some operations that are needed
#pragma warning(disable:4245) // in this example - we're working on bit level.
#endif                        // So we intentionally disable them.

//[large_bitset_cpp_includes
#include <limits>
#include "large_bitset.hpp"

using namespace std;
using namespace boost;
using namespace boost::icl;
using namespace mini;
//]


//[large_bitset_test_large_set_all
void test_large()
{ 
    const nat64 much = 0xffffffffffffffffull; 
    large_bitset<> venti; // ... the largest, I can think of ;)
    venti += discrete_interval<nat64>(0, much);

    cout << "----- Test function test_large() -----------------------------------------------\n";
    cout << "We have just turned on the awesome amount of 18,446,744,073,709,551,616 bits ;-)\n";
    venti.show_segments();
    //]

    //[large_bitset_test_large_erase_last
    cout << "---- Let's swich off the very last bit -----------------------------------------\n";
    venti -= much;
    venti.show_segments();

    cout << "---- Venti is plenty ... let's do something small: A tall ----------------------\n\n";
}
//]

//[large_bitset_test_small
void test_small()
{
    large_bitset<nat32, bits8> tall; // small is tall ...
        // ... because even this 'small' large_bitset 
        // can represent up to 2^32 == 4,294,967,296 bits.

    cout << "----- Test function test_small() -----------\n";
    cout << "-- Switch on all bits in range [0,64] ------\n";
    tall += discrete_interval<nat>(0, 64);
    tall.show_segments();
    cout << "--------------------------------------------\n";

    cout << "-- Turn off bits: 25,27,28 -----------------\n";

    (((tall -= 25) -= 27) -= 28) ;
    tall.show_segments();
    cout << "--------------------------------------------\n";

    cout << "-- Flip bits in range [24,30) --------------\n";
    tall ^= discrete_interval<nat>::right_open(24,30);
    tall.show_segments();
    cout << "--------------------------------------------\n";

    cout << "-- Remove the first 10 bits ----------------\n";
    tall -= discrete_interval<nat>::right_open(0,10);
    tall.show_segments();

    cout << "-- Remove even bits in range [0,72) --------\n";
    int bit;
    for(bit=0; bit<72; bit++) if(!(bit%2)) tall -= bit;
    tall.show_segments();

    cout << "--    Set odd  bits in range [0,72) --------\n";
    for(bit=0; bit<72; bit++) if(bit%2) tall += bit;
    tall.show_segments();

    cout << "--------------------------------------------\n\n";

}
//]

//[large_bitset_test_picturesque
void test_picturesque()
{
    typedef large_bitset<nat, bits8> Bit8Set;

    Bit8Set square, stare;
    square += discrete_interval<nat>(0,8);
    for(int i=1; i<5; i++)
    { 
        square += 8*i; 
        square += 8*i+7; 
    }

    square += discrete_interval<nat>(41,47);

    cout << "----- Test function test_picturesque() -----\n";
    cout << "-------- empty face:       " 
         << square.interval_count()           << " intervals -----\n";
    square.show_matrix(" *");

    stare += 18; stare += 21;
    stare += discrete_interval<nat>(34,38);

    cout << "-------- compressed smile: " 
         << stare.interval_count()            << " intervals -----\n";
    stare.show_matrix(" *");

    cout << "-------- staring bitset:   " 
         << (square + stare).interval_count() << " intervals -----\n";
    (square + stare).show_matrix(" *");

    cout << "--------------------------------------------\n";
}
//]

template<class NatT, class BitsT>
void test_set()
{
    const NatT much = (numeric_limits<NatT>::max)();

    large_bitset<NatT, BitsT> venti; //the largest, I can think of
    venti += discrete_interval<NatT>(0, much);

    cout << "--------------------------------------------------------------------------------\n";
    venti.show_segments();

    venti -= much;
    cout << "--------------------------------------------------------------------------------\n";
    venti.show_segments();
}



int main()
{
    cout << ">>Interval Container Library: Sample large_bitset.cpp <<\n";
    cout << "--------------------------------------------------------\n";
    test_large();
    test_small();
    test_picturesque();
    //test_set<nat64,bits64>();
    return 0;
}

