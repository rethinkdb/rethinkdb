.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. |//| replace:: **\\**
.. |/| replace:: ``\``

.. |default-root| replace:: ``C:\Program Files\boost\``\ |boost_ver|
.. |default-root-bold| replace:: **C:\\Program Files\\boost\\**\ |boost_ver-bold|

.. |root| replace:: *path\\to\\*\ |boost_ver|

.. |include-paths| replace:: Specific steps for setting up ``#include``
   paths in Microsoft Visual Studio follow later in this document;
   if you use another IDE, please consult your product's
   documentation for instructions.

.. |forward-slashes| replace:: Even Windows users can (and, for
   portability reasons, probably should) use forward slashes in
   ``#include`` directives; your compiler doesn't care.

.. |precompiled-dir| replace:: 

    **lib**\ |//| .....................\ *precompiled library binaries*


.. |windows-version-name-caveat| replace:: **On Windows, append a version
   number even if you only have one version installed** (unless you
   are using the msvc or gcc toolsets, which have special version
   detection code) or `auto-linking`_ will fail.

.. |command-line tool| replace:: `command-line tool`_

.. |pathsep| replace:: semicolon

.. |path| replace:: ``PATH``

.. |bootstrap| replace:: ``bootstrap.bat``

.. include:: common.rst
