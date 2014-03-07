
Iterators are generic means of addressing a particular element or a range 
of sequential elements in a sequence. They are also a mechanism that makes
it possible to decouple `algorithms`__ from concrete compile-time `sequence 
implementations`__. Under the hood, all MPL sequence algorithms are 
implemented in terms of iterators. In particular, that means that they 
will work on any custom compile-time sequence, given that the appropriate 
iterator inteface is provided.

__ `Algorithms`_
__ `label-Sequences-Classes`_

.. Analogy with STL iterators?
.. More?


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
