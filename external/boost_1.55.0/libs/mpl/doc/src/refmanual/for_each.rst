.. Algorithms/Runtime Algorithms//for_each |10

for_each
========

Synopsis
--------

.. parsed-literal::

    template<
          typename Sequence
        , typename F
        >
    void for_each( F f );

    template<
          typename Sequence
        , typename TransformOp
        , typename F
        >
    void for_each( F f );


Description
-----------

``for_each`` is a family of overloaded function templates:

* ``for_each<Sequence>( f )`` applies the runtime function object
  ``f`` to every element in the |begin/end<Sequence>| range.

* ``for_each<Sequence,TransformOp>( f )`` applies the runtime function
  object ``f`` to the result of the transformation ``TransformOp`` of
  every element in the |begin/end<Sequence>| range.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/for_each.hpp>


Parameters
----------

+-------------------+-----------------------------------+-----------------------------------+
| Parameter         | Requirement                       | Description                       |
+===================+===================================+===================================+
| ``Sequence``      | |Forward Sequence|                | A sequence to iterate.            |
+-------------------+-----------------------------------+-----------------------------------+
| ``TransformOp``   | |Lambda Expression|               | A transformation.                 |
+-------------------+-----------------------------------+-----------------------------------+
| ``f``             | An |unary function object|        | A runtime operation to apply.     |
+-------------------+-----------------------------------+-----------------------------------+


Expression semantics
--------------------

For any |Forward Sequence| ``s``, |Lambda Expression| ``op`` , and an
|unary function object| ``f``:

.. parsed-literal::

    for_each<s>( f );

:Return type:
    ``void``

:Postcondition:
    Equivalent to 
        
    .. parsed-literal::

        typedef begin<Sequence>::type i\ :sub:`1`;
        |value_initialized|\ < deref<i\ :sub:`1`>::type > x\ :sub:`1`;
        f(boost::get(x\ :sub:`1`));

        typedef next<i\ :sub:`1`>::type i\ :sub:`2`;
        |value_initialized|\ < deref<i\ :sub:`2`>::type > x\ :sub:`2`;
        f(boost::get(x\ :sub:`2`));
        |...|
        |value_initialized|\ < deref<i\ :sub:`n`>::type > x\ :sub:`n`;
        f(boost::get(x\ :sub:`n`));
        typedef next<i\ :sub:`n`>::type last; 
        
    where ``n == size<s>::value`` and ``last`` is identical to
    ``end<s>::type``; no effect if ``empty<s>::value == true``.


.. parsed-literal::

    for_each<s,op>( f );

:Return type:
    ``void``

:Postcondition:
    Equivalent to 
        
    .. parsed-literal::

        for_each< transform_view<s,op> >( f );


Complexity
----------

Linear. Exactly ``size<s>::value`` applications of ``op`` and ``f``.


Example
-------

.. parsed-literal::
    
    struct value_printer
    {
        template< typename U > void operator()(U x)
        {
            std::cout << x << '\\n';
        }
    };

    int main()
    {
        for_each< range_c<int,0,10> >( value_printer() );
    }


See also
--------

|Runtime Algorithms|, |Views|, |transform_view|

.. |unary function object| replace:: `unary function object <http://www.sgi.com/tech/stl/UnaryFunction.html>`__
.. |value_initialized| replace:: `value_initialized <http://www.boost.org/libs/utility/value_init.htm>`__


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
