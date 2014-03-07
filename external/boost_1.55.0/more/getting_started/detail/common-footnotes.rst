.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. [#warnings] Remember that warnings are specific to each compiler
   implementation.  The developer of a given Boost library might
   not have access to your compiler.  Also, some warnings are
   extremely difficult to eliminate in generic code, to the point
   where it's not worth the trouble.  Finally, some compilers don't
   have any source code mechanism for suppressing warnings.

.. [#distinct] This convention distinguishes the static version of
   a Boost library from the import library for an
   identically-configured Boost DLL, which would otherwise have the
   same name.

.. [#debug-abi] These libraries were compiled without optimization
   or inlining, with full debug symbols enabled, and without
   ``NDEBUG`` ``#define``\ d.  Although it's true that sometimes
   these choices don't affect binary compatibility with other
   compiled code, you can't count on that with Boost libraries.

.. [#native] This feature of STLPort is deprecated because it's
   impossible to make it work transparently to the user; we don't
   recommend it.

