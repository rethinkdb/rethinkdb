
The MPL includes a number of predefined metafunctions that can be roughly
classified in two categories: `general purpose metafunctions`, dealing with
conditional |type selection| and higher-order metafunction |invocation|, 
|composition|, and |argument binding|, and `numeric metafunctions`, 
incapsulating built-in and user-defined |arithmetic|, |comparison|, 
|logical|, and |bitwise| operations.

Given that it is possible to perform integer numeric computations at 
compile time using the conventional operators notation, the need for the 
second category might be not obvious, but it in fact plays a cental role in 
making programming with MPL seemingly effortless. In 
particular, there are at least two contexts where built-in language 
facilities fall short [#portability]_\ :

1) Passing a computation to an algorithm.
2) Performing a computation on non-integer data.

The second use case deserves special attention. In contrast to the built-in,
strictly integer compile-time arithmetics, the MPL numeric metafunctions are 
*polymorphic*, with support for *mixed-type arithmetics*. This means that they 
can operate on a variety of numeric types |--| for instance, rational, 
fixed-point or complex numbers, |--| and that, in general, you are allowed to 
freely intermix these types within a single expression. See |Numeric 
Metafunction| concept for more details on the MPL numeric infrastructure.

.. The provided `infrastructure`__ allows easy plugging of user-defined numeric 
   types
   Naturally, they also , meaning that you can perform a computation on the 
   arguments of different types, and the result will yeild the largest/most general 
   of them. For user-defined numeric types, they provide an `infrastructure`__ that 
   allows easy plugging and seemless integration with predefined library 
   types. details.

   __ `Numeric Metafunction`_


To reduce a negative syntactical impact of the metafunctions notation
over the infix operator notation, all numeric metafunctions
allow to pass up to N arguments, where N is defined by the value of
|BOOST_MPL_LIMIT_METAFUNCTION_ARITY| configuration macro.


.. [#portability] All other considerations aside, as of the time of this writing 
   (early 2004), using built-in operators on integral constants still often 
   present a portability problem |--| many compilers cannot handle particular 
   forms of expressions, forcing us to use conditional compilation. Because MPL
   numeric metafunctions work on types and encapsulate these kind of workarounds 
   internally, they elude these problems, so if you aim for portability, it is 
   generally adviced to use them in the place of the conventional operators, even 
   at the price of slightly decreased readability.


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
