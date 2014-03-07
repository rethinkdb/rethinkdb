.. Metafunctions/Miscellaneous//inherit_linearly |40

inherit_linearly
================

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Types
        , typename Node
        , typename Root = empty_base
        >
    struct inherit_linearly
        : fold<Types,Root,Node>
    {
    };


Description
-----------

A convenience wrapper for ``fold`` to use in the context of sequence-driven
class composition. Returns the result the successive application of binary
``Node`` to the result of the previous ``Node`` invocation (``Root`` if it's 
the first call) and every type in the |Forward Sequence| ``Types`` in order. 


Header
------

.. parsed-literal::
    
    #include <boost/mpl/inherit_linearly.hpp>


Model of
--------

|Metafunction|


Parameters
----------

+---------------+-------------------------------+---------------------------------------------------+
| Parameter     | Requirement                   | Description                                       |
+===============+===============================+===================================================+
| ``Types``     | |Forward Sequence|            | Types to inherit from.                            |
+---------------+-------------------------------+---------------------------------------------------+
| ``Node``      | Binary |Lambda Expression|    | A derivation metafunction.                        |
+---------------+-------------------------------+---------------------------------------------------+
| ``Root``      | A class type                  | A type to be placed at the root of the class      |
|               |                               | hierarchy.                                        |
+---------------+-------------------------------+---------------------------------------------------+


Expression semantics
--------------------

For any |Forward Sequence| ``types``, binary |Lambda Expression| ``node``, and arbitrary 
class type ``root``:


.. parsed-literal::

    typedef inherit_linearly<types,node,root>::type r; 

:Return type:
    A class type.

:Semantics:
    Equivalent to
        
    .. parsed-literal::

        typedef fold<types,root,node>::type r; 



Complexity
----------

Linear. Exactly ``size<types>::value`` applications of ``node``. 


Example
-------

.. parsed-literal::
    
    template< typename T > struct tuple_field
    {
        T field;
    };

    template< typename T >
    inline
    T& field(tuple_field<T>& t)
    {
        return t.field;
    }

    typedef inherit_linearly<
          vector<int,char const*,bool>
        , inherit< _1, tuple_field<_2> >
        >::type tuple;


    int main()
    {
        tuple t;
        
        field<int>(t) = -1;
        field<char const*>(t) = "text";
        field<bool>(t) = false;

        std::cout
            << field<int>(t) << '\n'
            << field<char const*>(t) << '\n'
            << field<bool>(t) << '\n'
            ;
    }


See also
--------

|Metafunctions|, |Algorithms|, |inherit|, |empty_base|, |fold|, |reverse_fold|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
