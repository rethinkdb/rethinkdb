.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

The Boost Distribution
======================

This is a sketch of the resulting directory structure:

.. parsed-literal::

 |boost_ver-bold|\ |//| .................\ *The “boost root directory”* 
    **index.htm** .........\ *A copy of www.boost.org starts here*
    **boost**\ |//| .........................\ *All Boost Header files*
    |precompiled-dir|    
    **libs**\ |//| ............\ *Tests, .cpp*\ s\ *, docs, etc., by library*
      **index.html** ........\ *Library documentation starts here*
      **algorithm**\ |//|
      **any**\ |//|
      **array**\ |//|
                      *…more libraries…*
    **status**\ |//| .........................\ *Boost-wide test suite*
    **tools**\ |//| ...........\ *Utilities, e.g. Boost.Build, quickbook, bcp*
    **more**\ |//| ..........................\ *Policy documents, etc.*
    **doc**\ |//| ...............\ *A subset of all Boost library docs*

.. sidebar:: Header Organization

   .. class:: pre-wrap

     The organization of Boost library headers isn't entirely uniform,
     but most libraries follow a few patterns:

     * Some older libraries and most very small libraries place all
       public headers directly into ``boost``\ |/|.

     * Most libraries' public headers live in a subdirectory of
       ``boost``\ |/|, named after the library.  For example, you'll find
       the Python library's ``def.hpp`` header in

       .. parsed-literal::

         ``boost``\ |/|\ ``python``\ |/|\ ``def.hpp``.

     * Some libraries have an “aggregate header” in ``boost``\ |/| that
       ``#include``\ s all of the library's other headers.  For
       example, Boost.Python_'s aggregate header is

       .. parsed-literal::

         ``boost``\ |/|\ ``python.hpp``.

     * Most libraries place private headers in a subdirectory called
       ``detail``\ |/|, or ``aux_``\ |/|.  Don't expect to find
       anything you can use in these directories.

It's important to note the following:

.. _Boost root directory:

1. The path to the **boost root directory** (often |default-root|) is
   sometimes referred to as ``$BOOST_ROOT`` in documentation and
   mailing lists .

2. To compile anything in Boost, you need a directory containing
   the ``boost``\ |/| subdirectory in your ``#include`` path.  |include-paths|

3. Since all of Boost's header files have the ``.hpp`` extension,
   and live in the ``boost``\ |/| subdirectory of the boost root, your
   Boost ``#include`` directives will look like:

   .. parsed-literal::

     #include <boost/\ *whatever*\ .hpp>

   or

   .. parsed-literal::

     #include "boost/\ *whatever*\ .hpp"

   depending on your preference regarding the use of angle bracket
   includes.  |forward-slashes|

4. Don't be distracted by the ``doc``\ |/| subdirectory; it only
   contains a subset of the Boost documentation.  Start with
   ``libs``\ |/|\ ``index.html`` if you're looking for the whole enchilada.

