.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

The zip iterator provides the ability to parallel-iterate
over several controlled sequences simultaneously. A zip 
iterator is constructed from a tuple of iterators. Moving
the zip iterator moves all the iterators in parallel.
Dereferencing the zip iterator returns a tuple that contains
the results of dereferencing the individual iterators. 
