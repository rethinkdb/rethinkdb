
.. _`Overloaded name`:

Overloaded name
    Overloaded name is a term used in this reference documentation to designate
    a metafunction providing more than one public interface. In reality, 
    class template overloading is nonexistent and the referenced functionality
    is implemented by other, unspecified, means.
    

.. _`Concept-identical`:

Concept-identical    
    A sequence ``s1`` is said to be concept-identical to a sequence ``s2`` if 
    ``s1`` and ``s2`` model the exact same set of concepts.


.. _`Bind expression`:

Bind expression
    A bind expression is simply that |--| an instantiation of one of the |bind| 
    class templates. For instance, these are all bind expressions::
    
        bind< quote3<if_>, _1,int,long >
        bind< _1, bind< plus<>, int_<5>, _2> >
        bind< times<>, int_<2>, int_<2> >

    and these are not::

        if_< _1, bind< plus<>, int_<5>, _2>, _2 >        
        protect< bind< quote3<if_>, _1,int,long > >
        _2


.. |overloaded name| replace:: `overloaded name`_
.. |concept-identical| replace:: `concept-identical`_
.. |bind expression| replace:: `bind expression`_


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
