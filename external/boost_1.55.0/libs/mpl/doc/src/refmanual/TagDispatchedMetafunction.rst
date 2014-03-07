.. Metafunctions/Concepts//Tag Dispatched Metafunction |50

Tag Dispatched Metafunction
===========================

Summary
-------

A |Tag Dispatched Metafunction| is a |Metafunction| that employs a
*tag dispatching* technique in its implementation to build an
infrastructure for easy overriding/extenstion of the metafunction's
behavior. 


Notation
--------

.. _`tag-metafunction`:

+---------------------------+-----------------------------------------------------------+
| Symbol                    | Legend                                                    |
+===========================+===========================================================+
| |``name``|                | A placeholder token for the specific metafunction's name. |
+---------------------------+-----------------------------------------------------------+
| |``tag-metafunction``|    | A placeholder token for the tag metafunction's name.      |
+---------------------------+-----------------------------------------------------------+
| |``tag``|                 | A placeholder token for one of possible tag types         |
|                           | returned by the tag metafunction.                         |
+---------------------------+-----------------------------------------------------------+

.. |``name``| replace:: *name*
.. |``tag-metafunction``| replace:: *tag-metafunction*
.. |``tag``| replace:: *tag*


Synopsis
--------

.. parsed-literal::

    template< typename Tag > struct *name*\_impl; 

    template<
          typename X
        *[, ...]*
        >
    struct *name*
        : *name*\_impl< typename *tag-metafunction*\<X>::type >
            ::template apply<X *[, ...]*>
    {
    };

    template< typename Tag > struct *name*\_impl
    {
        template< typename X *[, ...]* > struct apply
        {
            // *default implementation*
        };
    };

    template<> struct *name*\_impl<*tag*>
    {
        template< typename X *[, ...]* > struct apply
        {
            // *tag-specific implementation*
        };
    };


Description
-----------

The usual mechanism for overriding a metafunction's behavior is class 
template specialization |--| given a library-defined metafunction ``f``,
it's possible to write a specialization of ``f`` for a specific type 
``user_type`` that would have the required semantics [#spec]_.

While this mechanism is always available, it's not always the most
convenient one, especially if it is desirable to specialize a 
metafunction's behavior for a *family* of related types. A typical 
example of it is numbered forms of sequence classes in MPL itself 
(``list0``, ..., ``list50``, et al.), and sequence classes in general.

A |Tag Dispatched Metafunction| is a concept name for an instance of
the metafunction implementation infrastructure being employed by the
library to make it easier for users and implementors to override the
behavior of library's metafunctions operating on families of specific
types.

The infrastructure is built on a variation of the technique commonly
known as *tag dispatching* (hence the concept name), 
and involves three entities: a metafunction itself, an associated 
tag-producing |tag-metafunction|, and the metafunction's 
implementation, in the form of a |Metafunction Class| template 
parametrized by a ``Tag`` type parameter. The metafunction redirects
to its implementation class template by invoking its specialization 
on a tag type produced by the tag metafunction with the original 
metafunction's parameters.


.. [#spec] Usually such user-defined specialization is still required 
   to preserve the ``f``'s original invariants and complexity requirements.


Example
-------

.. parsed-literal::

   #include <boost/mpl/size.hpp>

   namespace user {

   struct bitset_tag;

   struct bitset0
   {
       typedef bitset_tag tag;
       // ...
   };

   template< typename B0 > struct bitset1
   {
       typedef bitset_tag tag;
       // ...
   };

   template< typename B0, *...,* typename B\ *n* > struct bitset\ *n*
   {
       typedef bitset_tag tag;
       // ...
   };

   } // namespace user

   namespace boost { namespace mpl {
   template<> struct size_impl<user::bitset_tag>
   {
       template< typename Bitset > struct apply
       {
           typedef typename Bitset::size type;
       };
   };
   }}


Models
-------

* |sequence_tag|


See also
--------

|Metafunction|, |Metafunction Class|, |Numeric Metafunction|


.. |tag-metafunction| replace:: `tag metafunctions`_
.. _`tag metafunctions`: `tag-metafunction`_

.. |tag dispatched| replace:: `tag dispatched`_
.. _`tag dispatched`: `Tag Dispatched Metafunction`_


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
