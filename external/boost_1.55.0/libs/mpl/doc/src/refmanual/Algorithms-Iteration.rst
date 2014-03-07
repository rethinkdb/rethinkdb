
Iteration algorithms are the basic building blocks behind many of the 
MPL's algorithms, and are usually the first place to look at when 
starting to build a new one. Abstracting away the details of sequence 
iteration and employing various optimizations such as recursion 
unrolling, they provide significant advantages over a hand-coded 
approach.

..  Of all of iteration algorithms, ``iter_fold_if`` is the 
    most complex and at the same time the most fundamental. The rest of 
    the algorithms from the category |--| ``iter_fold``, ``reverse_iter_fold``,
    ``fold``, and ``reverse_fold`` |--| simply provide a more high-level 
    (and more restricted) interface to the core ``iter_fold_if`` 
    functionality [#performace]_.

    .. [#performace] That's not to say that they are *implemented*
    through ``iter_fold_if`` |--| they are often not, in particular 
    because the restricted functionality allows for more 
    optimizations.


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
