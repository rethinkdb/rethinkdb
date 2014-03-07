
According to their name, MPL's *transformation*, or *sequence-building
algorithms* provide the tools for building new sequences from the existing
ones by performing some kind of transformation. A typical transformation 
alogrithm takes one or more input sequences and a transformation 
metafunction/predicate, and returns a new sequence built according to the 
algorithm's semantics through the means of its |Inserter| argument, which 
plays a role similar to the role of run-time |Output Iterator|. 

.. Say something about optionality of Inserters/their default behavior

Every transformation algorithm is a |Reversible Algorithm|, providing 
an accordingly named ``reverse_`` counterpart carrying the transformation 
in the reverse order. Thus, all sequence-building algorithms come in pairs,
for instance ``replace`` / ``reverse_replace``. In presence of variability of
the output sequence's properties such as front or backward extensibility, 
the existence of the bidirectional algorithms allows for the most efficient 
way to perform the required transformation.

.. |Transformation Algorithms| replace:: `Transformation Algorithms`_

.. |transformation algorithm| replace:: `transformation algorithm`_
.. _transformation algorithm: `Transformation Algorithms`_
.. |transformation algorithms| replace:: `transformation algorithms`_


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
