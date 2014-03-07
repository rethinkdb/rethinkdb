///////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

#include <boost/multiprecision/cpp_int.hpp>
#include <iostream>
#include <iomanip>
#include <vector>

//[FAC1

/*`
In this simple example, we'll write a routine to print out all of the factorials
which will fit into a 128-bit integer.  At the end of the routine we do some
fancy iostream formatting of the results:
*/
/*=
#include <boost/multiprecision/cpp_int.hpp>
#include <iostream>
#include <iomanip>
#include <vector>
*/

void print_factorials()
{
   using boost::multiprecision::cpp_int;
   //
   // Print all the factorials that will fit inside a 128-bit integer.
   //
   // Begin by building a big table of factorials, once we know just how 
   // large the largest is, we'll be able to "pretty format" the results.
   //
   // Calculate the largest number that will fit inside 128 bits, we could
   // also have used numeric_limits<int128_t>::max() for this value:
   cpp_int limit = (cpp_int(1) << 128) - 1;
   // 
   // Our table of values:
   std::vector<cpp_int> results;
   //
   // Initial values:
   unsigned i = 1;
   cpp_int factorial = 1;
   //
   // Cycle through the factorials till we reach the limit:
   while(factorial < limit)
   {
      results.push_back(factorial);
      ++i;
      factorial *= i;
   }
   //
   // Lets see how many digits the largest factorial was:
   unsigned digits = results.back().str().size();
   //
   // Now print them out, using right justification, while we're at it
   // we'll indicate the limit of each integer type, so begin by defining
   // the limits for 16, 32, 64 etc bit integers:
   cpp_int limits[] = { 
      (cpp_int(1) << 16) - 1,
      (cpp_int(1) << 32) - 1,
      (cpp_int(1) << 64) - 1,
      (cpp_int(1) << 128) - 1,
   };
   std::string bit_counts[] = { "16", "32", "64", "128" };
   unsigned current_limit = 0;
   for(unsigned j = 0; j < results.size(); ++j)
   {
      if(limits[current_limit] < results[j])
      {
         std::string message = "Limit of " + bit_counts[current_limit] + " bit integers";
         std::cout << std::setfill('.') << std::setw(digits+1) << std::right << message << std::setfill(' ') << std::endl;
         ++current_limit;
      }
      std::cout << std::setw(digits + 1) << std::right << results[j] << std::endl;
   }
}

/*`
The output from this routine is:
[template nul[]] [# fix for quickbook bug]
[pre [nul]
                                       1
                                       2
                                       6
                                      24
                                     120
                                     720
                                    5040
                                   40320
................Limit of 16 bit integers
                                  362880
                                 3628800
                                39916800
                               479001600
................Limit of 32 bit integers
                              6227020800
                             87178291200
                           1307674368000
                          20922789888000
                         355687428096000
                        6402373705728000
                      121645100408832000
                     2432902008176640000
................Limit of 64 bit integers
                    51090942171709440000
                  1124000727777607680000
                 25852016738884976640000
                620448401733239439360000
              15511210043330985984000000
             403291461126605635584000000
           10888869450418352160768000000
          304888344611713860501504000000
         8841761993739701954543616000000
       265252859812191058636308480000000
      8222838654177922817725562880000000
    263130836933693530167218012160000000
   8683317618811886495518194401280000000
 295232799039604140847618609643520000000 
]
*/

//]

//[BITOPS

/*` 
In this example we'll show how individual bits within an integer may be manipulated, 
we'll start with an often needed calculation of ['2[super n] - 1], which we could obviously
implement like this:
*/

using boost::multiprecision::cpp_int;

cpp_int b1(unsigned n)
{
   cpp_int r(1);
   return (r << n) - 1;
}

/*`
Calling:

   std::cout << std::hex << std::showbase << b1(200) << std::endl;

Yields as expected:

[pre 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF]

However, we could equally just set the n'th bit in the result, like this:
*/

cpp_int b2(unsigned n)
{
   cpp_int r(0);
   return --bit_set(r, n);
}

/*`
Note how the `bit_set` function sets the specified bit in its argument and then returns a reference to the result -
which we can then simply decrement.  The result from a call to `b2` is the same as that to `b1`.

We can equally test bits, so for example the n'th bit of the result returned from `b2` shouldn't be set
unless we increment it first:

   assert(!bit_test(b1(200), 200));     // OK
   assert(bit_test(++b1(200), 200));    // OK

And of course if we flip the n'th bit after increment, then we should get back to zero:

   assert(!bit_flip(++b1(200), 200));   // OK
*/

//]

int main()
{
   print_factorials();

   std::cout << std::hex << std::showbase << b1(200) << std::endl;
   std::cout << std::hex << std::showbase << b2(200) << std::endl;
   assert(!bit_test(b1(200), 200));  // OK
   assert(bit_test(++b1(200), 200));    // OK
   assert(!bit_flip(++b1(200), 200));   // OK
   return 0;
}

/*

Program output:

                                       1
                                       2
                                       6
                                      24
                                     120
                                     720
                                    5040
                                   40320
................Limit of 16 bit integers
                                  362880
                                 3628800
                                39916800
                               479001600
................Limit of 32 bit integers
                              6227020800
                             87178291200
                           1307674368000
                          20922789888000
                         355687428096000
                        6402373705728000
                      121645100408832000
                     2432902008176640000
................Limit of 64 bit integers
                    51090942171709440000
                  1124000727777607680000
                 25852016738884976640000
                620448401733239439360000
              15511210043330985984000000
             403291461126605635584000000
           10888869450418352160768000000
          304888344611713860501504000000
         8841761993739701954543616000000
       265252859812191058636308480000000
      8222838654177922817725562880000000
    263130836933693530167218012160000000
   8683317618811886495518194401280000000
 295232799039604140847618609643520000000 
 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
 */
