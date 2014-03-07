
The MPL provides a number of |Trivial Metafunction|\ s that a nothing more than
thin wrappers for a differently-named class nested type members. While important
in the context of `in-place metafunction composition`__, these metafunctions have
so little to them that presenting them in the same format as the rest of the
compoments in this manual would result in more boilerplate syntactic baggage than
the actual content. To avoid this problem, we instead factor out the common 
metafunctions' requirements into the `corresponding concept`__ and gather all of 
them in a single place |--| this subsection |--| in a compact table form that is 
presented below.

__ `Composition and Argument Binding`_
__ `Trivial Metafunction`_

.. |Trivial Metafunctions| replace:: `Trivial Metafunctions`_
.. _`Trivial Metafunctions`: `label-Metafunctions-Trivial`_


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
