
The taxonomy of sequence concepts in MPL parallels the taxonomy of the MPL 
|iterators|, with two additional classification dimensions: 
`extensibility` and `associativeness`. 

.. The latter two are orthogonal to 
   sequence traversal characteristics, but not to each other, meaning that 
   a sequence can be characterized as both `Bidirectional`__ 
   and `Back Extensible`__, or `Bidirectional`__ and 
   `Extensible Associative`__, but not as `Bidirectional`__, 
   `Back Extensible`__ *and* `Extensible Associative`__.

   __ `Bidirectional Sequence`_
   __ `Back Extensible Sequence`_
   __ `Bidirectional Sequence`_
   __ `Extensible Associative Sequence`_
   __ `Bidirectional Sequence`_
   __ `Back Extensible Sequence`_
   __ `Extensible Associative Sequence`_


   Two utility concepts, |Variadic Sequence| and |Integral Sequence Wrapper|,
   are not applicable in generic contexts, but are used to group together
   the common parts of different sequence classes' specifications.


.. |sequence concepts| replace:: `sequence concepts`_ 
.. _`sequence concepts`: `label-Sequences-Concepts`_


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
