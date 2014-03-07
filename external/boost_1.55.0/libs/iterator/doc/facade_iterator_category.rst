.. |iterator-category| replace:: *iterator-category*
.. _iterator-category:

.. parsed-literal::
  
  *iterator-category*\ (C,R,V) :=
     if (C is convertible to std::input_iterator_tag
         || C is convertible to std::output_iterator_tag
     )
         return C

     else if (C is not convertible to incrementable_traversal_tag)
         *the program is ill-formed*

     else return a type X satisfying the following two constraints:

        1. X is convertible to X1, and not to any more-derived
           type, where X1 is defined by:

             if (R is a reference type
                 && C is convertible to forward_traversal_tag)
             {
                 if (C is convertible to random_access_traversal_tag)
                     X1 = random_access_iterator_tag
                 else if (C is convertible to bidirectional_traversal_tag)
                     X1 = bidirectional_iterator_tag
                 else
                     X1 = forward_iterator_tag
             }
             else
             {
                 if (C is convertible to single_pass_traversal_tag
                     && R is convertible to V)
                     X1 = input_iterator_tag
                 else
                     X1 = C
             }

        2. |category-to-traversal|_\ (X) is convertible to the most
           derived traversal tag type to which X is also
           convertible, and not to any more-derived traversal tag
           type.

.. |category-to-traversal| replace:: *category-to-traversal*
.. _`category-to-traversal`: new-iter-concepts.html#category-to-traversal

[Note: the intention is to allow ``iterator_category`` to be one of
the five original category tags when convertibility to one of the
traversal tags would add no information]

.. Copyright David Abrahams 2004. Use, modification and distribution is
.. subject to the Boost Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
