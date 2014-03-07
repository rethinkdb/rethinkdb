.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

The adaptor takes two arguments:

  * an iterator to the range V on which the permutation
    will be applied
  * the reindexing scheme that defines how the
    elements of V will be permuted.

Note that the permutation iterator is not limited to strict
permutations of the given range V.  The distance between begin and end
of the reindexing iterators is allowed to be smaller compared to the
size of the range V, in which case the permutation iterator only
provides a permutation of a subrange of V.  The indexes neither need
to be unique. In this same context, it must be noted that the past the
end permutation iterator is completely defined by means of the
past-the-end iterator to the indices.
