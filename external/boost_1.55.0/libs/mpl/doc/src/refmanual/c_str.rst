.. Metafunctions/String Operations//c_str |10


c_str
=====

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        >
    struct c_str
    {
        typedef |unspecified| type;
        static char const value[];
    };


Description
-----------

``c_str`` converts the |Forward Sequence| of |Integral Constant|\ s ``Sequence``
into a null-terminated byte string containing an equivalent sequence.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/string.hpp>


Model of
--------

|Metafunction|


Parameters
----------

+---------------+---------------------------+-----------------------------------------------+
| Parameter     | Requirement               | Description                                   |
+===============+===========================+===============================================+
| ``Sequence``  | |Forward Sequence| of     | A sequence to be converted into a             |
|               | |Integral Constant|\ s    | null-terminated byte string.                  |
+---------------+---------------------------+-----------------------------------------------+


Expression semantics
--------------------

.. compound::
    :class: expression-semantics

    For any |Forward Sequence| of |Integral Constant|\ s ``s``,

    .. parsed-literal::

        c_str<s>::value; 

    :Return type:
        A null-terminated byte string.

    :Precondition:
        ``size<s>::value <= BOOST_MPL_STRING_MAX_LENGTH``.

    :Semantics:
        Equivalent to 
        
        .. parsed-literal::
           
           char const value[] = {
               at<s, 0>::type::value
             , ...
             , at<s, size<s>::value-1>::type::value
             , '\\0'
           };

Complexity
----------

+-------------------------------+-----------------------------------+
| Sequence archetype            | Complexity                        |
+===============================+===================================+
| |Forward Sequence|            | Linear.                           |
+-------------------------------+-----------------------------------+

Example
-------

.. parsed-literal::
    
    typedef vector_c<char,'h','e','l','l','o'> hello;
    assert( 0 == std::strcmp( c_str<hello>::value, "hello" ) );

See also
--------

|Forward Sequence|, |Integral Constant|, |string|


.. copyright:: Copyright ©  2009 Eric Niebler
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
