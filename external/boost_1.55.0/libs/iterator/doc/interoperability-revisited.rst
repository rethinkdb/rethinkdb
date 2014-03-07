++++++++++++++++++++++++++++
 Interoperability Revisited 
++++++++++++++++++++++++++++

:date: $Date: 2008-03-22 14:45:55 -0700 (Sat, 22 Mar 2008) $
:copyright: Copyright Thomas Witt 2004.

.. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

Problem
=======

The current iterator_facade specification makes it unneccessarily tedious to
implement interoperable iterators.

In the following text a simplified example of the current iterator_facade specification is used to
illustrate the problem.

In the current specification binary operators are implemented in the following way::

  template <class Derived>
  struct Facade
  {
  };

  template <class T1, T2>
  struct is_interoperable :
    or_< 
         is_convertible<T1, T2>
       , is_convertible<T2, T1>
    > 
  {};

  template<
      class Derived1
    , class Derived2
  >
  enable_if<is_interoperable<Derived1, Derived2>, bool> operator==(
      Derived1 const& lhs
    , Derived2 const& rhs
  )
  {
    return static_cast<Derived1 const&>(lhs).equal_to(static_cast<Derived2 const&(rhs));
  } 

The problem with this is that operator== always forwards to Derived1::equal_to. The net effect is that the
following "obvious" implementation of to interoperable types does
not quite work. ::

  struct Mutable : Facade<Mutable>
  {
    bool equal_to(Mutable const&);  
  };

  struct Constant : Facade<Constant>
  {
    Constant();
    Constant(Constant const&);
    Constant(Mutable const&);

    ...

    bool equal_to(Constant const&);  
  };

  Constant c;
  Mutable  m;

  c == m; // ok, dispatched to Constant::equal_to
  m == c; // !! error, dispatched to Mutable::equal_to

  Instead the following "slightly" more complicated implementation is necessary

  struct Mutable : Facade<Mutable>
  {
    template <class T>
    enable_if<is_convertible<Mutable, T> || is_convertible<T, Mutable>, bool>::type equal_to(T const&);  
  };

  struct Constant : Tag<Constant>
  {
    Constant();
    Constant(Constant const&);
    Constant(Mutable const&);

    template <class T>
    enable_if<is_convertible<Constant, T> || is_convertible<T, Constant>, bool>::type equal_to(T const&);  
  };

Beside the fact that the code is significantly more complex to understand and to teach there is
a major design problem lurking here. Note that in both types equal_to is a function template with 
an unconstrained argument T. This is necessary so that further types can be made interoperable with
Mutable or Constant. Would Mutable be defined as   ::

  struct Mutable : Facade<Mutable>
  {
    bool equal_to(Mutable const&);  
    bool equal_to(Constant const&);  
  };

Constant and Mutable would still be interoperable but no further interoperable could be added 
without changing Mutable. Even if this would be considered acceptable the current specification forces
a two way dependency between interoperable types. Note in the templated equal_to case this dependency 
is implicitly created when specializing equal_to.

Solution
========

The two way dependency can be avoided by enabling type conversion in the binary operator
implementation. Note that this is the usual way interoperability betwween types is achieved
for binary operators and one reason why binary operators are usually implemented as non-members.

A simple implementation of this strategy would look like this ::

  template<
      class T1
    , class T2
  >
  struct interoperable_base :
      if_< 
          is_convertible<
              T2
            , T1
          >
        , T1
        , T2>
  {};


  template<
      class Derived1
    , class Derived2
  >
  enable_if<is_interoperable<Derived1, Derived2>, bool> operator==(
      Derived1 const& lhs
    , Derived2 const& rhs
  )
  {
    typedef interoperable_base<
                Derived1
              , Derived2
            >::type Base;

    return static_cast<Base const&>(lhs).equal_to(static_cast<Derived2 const&(rhs));
  } 

This way our original simple and "obvious" implementation would
work again. ::

  c == m; // ok, dispatched to Constant::equal_to
  m == c; // ok, dispatched to Constant::equal_to, m converted to Constant

The backdraw of this approach is that a possibly costly conversion of iterator objects
is forced on the user even in cases where direct comparison could be implemented
in a much more efficient way. This problem arises especially for iterator_adaptor
specializations and can be significantly slow down the iteration over ranges. Given the fact
that iteration is a very basic operation this possible performance degradation is not 
acceptable.

Luckily whe can have our cake and eat it by a slightly more clever implementation of the binary 
operators. ::

  template<
      class Derived1
    , class Derived2
  >
  enable_if<is_convertible<Derived2, Derived1>, bool> operator==(
      Derived1 const& lhs
    , Derived2 const& rhs
  )
  {
    return static_cast<Derived1 const&>(lhs).equal_to(static_cast<Derived2 const&(rhs));
  } 

  template<
      class Derived1
    , class Derived2
  >
  enable_if<is_convertible<Derived1, Derived2>, bool> operator==(
      Derived1 const& lhs
    , Derived2 const& rhs
  )
  {
    return static_cast<Derived2 const&>(rhs).equal_to(static_cast<Derived1 const&(lhs));
  } 

Given our simple and obvious definition of Mutable and Constant nothing has changed yet. ::

  c == m; // ok, dispatched to Constant::equal_to, m converted to Constant
  m == c; // ok, dispatched to Constant::equal_to, m converted to Constant

But now the user can avoid the type conversion by supplying the
appropriate overload in Constant :: 

  struct Constant : Facade<Constant>
  {
    Constant();
    Constant(Constant const&);
    Constant(Mutable const&);

    ...

    bool equal_to(Constant const&);  
    bool equal_to(Mutable const&);  
  };

  c == m; // ok, dispatched to Constant::equal_to(Mutable const&), no conversion
  m == c; // ok, dispatched to Constant::equal_to(Mutable const&), no conversion

This definition of operator== introduces a possible ambiguity when both types are convertible
to each other. I don't think this is a problem as this behaviour is the same with concrete types.
I.e.  ::

  struct A {};

  bool operator==(A, A);

  struct B { B(A); }; 

  bool operator==(B, B);

  A a;
  B b(a);

  a == b; // error, ambiguous overload

Effect
======

Iterator implementations using iterator_facade look exactly as if they were
"hand-implemented" (I am working on better wording).

a) Less burden for the user

b) The definition (standardese) of specialized adpters might be easier 
   (This has to be proved yet)
