.. Metafunctions/Miscellaneous//numeric_cast |50

numeric_cast
============

Synopsis
--------

.. parsed-literal::

    template< 
          typename SourceTag
        , typename TargetTag
        >
    struct numeric_cast;


Description
-----------

Each ``numeric_cast`` specialization is a user-specialized unary |Metafunction Class| 
providing a conversion between two numeric types.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/numeric_cast.hpp>


Parameters
----------

+---------------+---------------------------+-----------------------------------------------+
| Parameter     | Requirement               | Description                                   |
+===============+===========================+===============================================+
| ``SourceTag`` | |Integral Constant|       | A tag for the conversion's source type.       |
+---------------+---------------------------+-----------------------------------------------+
| ``TargetTag`` | |Integral Constant|       | A tag for the conversion's destination type.  |
+---------------+---------------------------+-----------------------------------------------+


Expression semantics
--------------------

If ``x`` and ``y`` are two numeric types, ``x`` is convertible to ``y``, and 
``x_tag`` and ``y_tag`` are the types' corresponding |Integral Constant| tags:


.. parsed-literal::

    typedef apply_wrap\ ``2``\< numeric_cast<x_tag,y_tag>,x >::type  r;

:Return type:
    A type.

:Semantics:
    ``r`` is a value of ``x`` converted to the type of ``y``.


Complexity
----------

Unspecified.


Example
-------

.. parsed-literal::
    
    struct complex_tag : int_<10> {};

    template< typename Re, typename Im > struct complex
    {
        typedef complex_tag tag;
        typedef complex type;
        typedef Re real;
        typedef Im imag;
    };

    template< typename C > struct real : C::real {};
    template< typename C > struct imag : C::imag {};

    namespace boost { namespace mpl {

    template<> struct numeric_cast< integral_c_tag,complex_tag >
    {
        template< typename N > struct apply
            : complex< N, integral_c< typename N::value_type, 0 > >
        {
        };
    };

    template<>
    struct plus_impl< complex_tag,complex_tag >
    {
        template< typename N1, typename N2 > struct apply
            : complex<
                  plus< typename N1::real, typename N2::real >
                , plus< typename N1::imag, typename N2::imag >
                >
        {
        };
    };

    }}

    typedef int_<2> i;
    typedef complex< int_<5>, int_<-1> > c1;
    typedef complex< int_<-5>, int_<1> > c2;

    typedef plus<c1,i> r4;
    BOOST_MPL_ASSERT_RELATION( real<r4>::value, ==, 7 );
    BOOST_MPL_ASSERT_RELATION( imag<r4>::value, ==, -1 );

    typedef plus<i,c2> r5;
    BOOST_MPL_ASSERT_RELATION( real<r5>::value, ==, -3 );
    BOOST_MPL_ASSERT_RELATION( imag<r5>::value, ==, 1 );


See also
--------

|Metafunctions|, |Numeric Metafunction|, |plus|, |minus|, |times|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
