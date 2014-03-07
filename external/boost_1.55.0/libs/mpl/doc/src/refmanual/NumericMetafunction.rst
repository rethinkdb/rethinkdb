.. Metafunctions/Concepts//Numeric Metafunction |60

Numeric Metafunction
====================

Description
-----------

A |Numeric Metafunction| is a |tag dispatched metafunction| that provides 
a built-in infrastructure for easy implementation of mixed-type operations.


Expression requirements
-----------------------

|In the following table...| ``op`` is a placeholder token for the actual 
|Numeric Metafunction|'s name, and ``x``, ``y`` and |x1...xn| are
arbitrary numeric types.

+-------------------------------------------+-----------------------+---------------------------+
| Expression                                | Type                  | Complexity                |
+===========================================+=======================+===========================+
|``op_tag<x>::type``                        | |Integral Constant|   | Amortized constant time.  |
+-------------------------------------------+-----------------------+---------------------------+
| .. parsed-literal::                       | Any type              | Unspecified.              |
|                                           |                       |                           |
|    op_impl<                               |                       |                           |
|         op_tag<x>::type                   |                       |                           |
|       , op_tag<y>::type                   |                       |                           |
|       >::apply<x,y>::type                 |                       |                           |
+-------------------------------------------+-----------------------+---------------------------+
|``op<``\ |x1...xn|\ ``>::type``            | Any type              | Unspecified.              |
+-------------------------------------------+-----------------------+---------------------------+


Expression semantics
--------------------

.. parsed-literal::

    typedef op_tag<x>::type tag;

:Semantics:
    ``tag`` is a tag type for ``x`` for ``op``. 
    ``tag::value`` is ``x``\ 's *conversion rank*.


.. ..........................................................................

.. parsed-literal::

    typedef op_impl<
          op_tag<x>::type
        , op_tag<y>::type
        >::apply<x,y>::type r;

:Semantics:
    ``r`` is the result of ``op`` application on arguments ``x`` 
    and ``y``.


.. ..........................................................................

.. parsed-literal::

    typedef op<\ |x1...xn|\ >::type r;

:Semantics:
    ``r`` is the result of ``op`` application on arguments |x1...xn|.




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

    typedef complex< int_<5>, int_<-1> > c1;
    typedef complex< int_<-5>, int_<1> > c2;

    typedef plus<c1,c2> r1;
    BOOST_MPL_ASSERT_RELATION( real<r1>::value, ==, 0 );
    BOOST_MPL_ASSERT_RELATION( imag<r1>::value, ==, 0 );

    typedef plus<c1,c1> r2;
    BOOST_MPL_ASSERT_RELATION( real<r2>::value, ==, 10 );
    BOOST_MPL_ASSERT_RELATION( imag<r2>::value, ==, -2 );

    typedef plus<c2,c2> r3;
    BOOST_MPL_ASSERT_RELATION( real<r3>::value, ==, -10 );
    BOOST_MPL_ASSERT_RELATION( imag<r3>::value, ==, 2 );



Models
------

* |plus|
* |minus|
* |times|
* |divides|


See also
--------

|Tag Dispatched Metafunction|, |Metafunctions|, |numeric_cast|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
