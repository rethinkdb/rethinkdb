
Compile-time sequences of types are one of the basic concepts of C++ 
template metaprogramming. Differences in types of objects being 
manipulated is the most common point of variability of similar, but 
not identical designs, and these are a direct target for 
metaprogramming. Templates were originally designed to address this
exact problem. However, without predefined mechanisms for 
representing and manipulating *sequences* of types as opposed to 
standalone template parameters, high-level template metaprogramming
is severely limited in its capabitilies.

The MPL recognizes the importance of type sequences as a fundamental
building block of many higher-level metaprogramming designs by
providing us with a conceptual framework for formal reasoning 
and understanding of sequence properties, guarantees and 
characteristics, as well as a first-class implementation of that
framework |--| a wealth of tools for concise, convenient,
conceptually precise and efficient sequence manipulation.


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
