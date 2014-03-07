.. Iterators/Iterator Metafunctions//iterator_category |60

iterator_category
=================

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Iterator
        >
    struct iterator_category
    {
        typedef typename Iterator::category type;
    };



Description
-----------

Returns one of the following iterator category tags: 

* ``forward_iterator_tag``
* ``bidirectional_iterator_tag``
* ``random_access_iterator_tag``


Header
------

.. parsed-literal::
    
    #include <boost/mpl/iterator_category.hpp>
    #include <boost/mpl/iterator_tags.hpp>


Parameters
----------

+---------------+-----------------------+-------------------------------------------+
| Parameter     | Requirement           | Description                               |
+===============+=======================+===========================================+
| ``Iterator``  | |Forward Iterator|    | The iterator to obtain a category for.    |
+---------------+-----------------------+-------------------------------------------+


Expression semantics
--------------------

For any |Forward Iterator|\ s ``iter``:


.. parsed-literal::

    typedef iterator_category<iter>::type tag; 

:Return type:
    |Integral Constant|.

:Semantics:
    ``tag`` is ``forward_iterator_tag`` if ``iter`` is a model of |Forward Iterator|, 
    ``bidirectional_iterator_tag`` if ``iter`` is a model of |Bidirectional Iterator|,
    or ``random_access_iterator_tag`` if ``iter`` is a model of |Random Access Iterator|;

:Postcondition:
     ``forward_iterator_tag::value < bidirectional_iterator_tag::value``,
      ``bidirectional_iterator_tag::value < random_access_iterator_tag::value``.


Complexity
----------

Amortized constant time.


Example
-------

.. parsed-literal::

    template< typename Tag, typename Iterator >
    struct algorithm_impl
    {
        // *O(n)* implementation
    };

    template< typename Iterator >
    struct algorithm_impl<random_access_iterator_tag,Iterator>
    {
        // *O(1)* implementation
    };
    
    template< typename Iterator >
    struct algorithm
        : algorithm_impl<
              iterator_category<Iterator>::type
            , Iterator
            >
    {
    };



See also
--------

|Iterators|, |begin| / |end|, |advance|, |distance|, |next|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
