.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

=============================================
 |(logo)|__ Getting Started on Unix Variants
=============================================

.. meta::
    :description: Getting Started with Boost on Unix Variants (including Linux and MacOS)

.. |(logo)| image:: ../../boost.png
   :alt: Boost
   :class: boost-logo

__ ../../index.htm

.. section-numbering::

.. maybe we don't need this
   .. Admonition:: A note to Cygwin_ and MinGW_ users

     If you plan to build from the Cygwin_ bash shell, you're in the
     right place.  If you plan to use your tools from the Windows
     command prompt, you should follow the instructions for `getting
     started on Windows`_.  Other command shells, such as MinGW_\ 's
     MSYS, are not supported—they may or may not work.

     .. _`Getting Started on Windows`: windows.html
     .. _Cygwin: http://www.cygwin.com
     .. _MinGW: http://mingw.org

.. Contents:: Index

Get Boost
=========

The most reliable way to get a copy of Boost is to download a
distribution from SourceForge_:

.. _SourceForge: `sf-download`_

1. Download |boost.tar.bz2|_.  

2. In the directory where you want to put the Boost installation,
   execute

   .. parsed-literal::

      tar --bzip2 -xf */path/to/*\ |boost_ver|\ .tar.bz2

.. |boost.tar.bz2| replace:: |boost_ver|\ ``.tar.bz2``

.. _`boost.tar.bz2`: `sf-download`_

.. Admonition:: Other Packages

   RedHat, Debian, and other distribution packagers supply Boost
   library packages, however you may need to adapt these
   instructions if you use third-party packages, because their
   creators usually choose to break Boost up into several packages,
   reorganize the directory structure of the Boost distribution,
   and/or rename the library binaries. [#packagers]_ If you have
   any trouble, we suggest using an official Boost distribution
   from SourceForge_.

.. include:: detail/distro.rst

.. include:: detail/header-only.rst

.. include:: detail/build-simple-head.rst

Now, in the directory where you saved ``example.cpp``, issue the
following command:

.. parsed-literal::

  c++ -I |root| example.cpp -o example

To test the result, type:

.. parsed-literal::

  echo 1 2 3 | ./example

.. include:: detail/errors-and-warnings.rst

.. include:: detail/binary-head.rst

Easy Build and Install
----------------------

Issue the following commands in the shell (don't type ``$``; that
represents the shell's prompt):

.. parsed-literal::

  **$** cd |root|
  **$** ./bootstrap.sh --help

Select your configuration options and invoke ``./bootstrap.sh`` again
without the ``--help`` option.  Unless you have write permission in
your system's ``/usr/local/`` directory, you'll probably want to at
least use

.. parsed-literal::

  **$** ./bootstrap.sh **--prefix=**\ *path*\ /\ *to*\ /\ *installation*\ /\ *prefix* 

to install somewhere else.  Also, consider using the
``--show-libraries`` and ``--with-libraries=``\ *library-name-list* options to limit the
long wait you'll experience if you build everything.  Finally,

.. parsed-literal::

  **$** ./b2 install

will leave Boost binaries in the ``lib/`` subdirectory of your
installation prefix.  You will also find a copy of the Boost
headers in the ``include/`` subdirectory of the installation
prefix, so you can henceforth use that directory as an ``#include``
path in place of the Boost root directory.

|next|__

__ `Link Your Program to a Boost Library`_

Or, Build Custom Binaries
-------------------------

If you're using a compiler other than your system's default, you'll
need to use Boost.Build_ to create binaries.

You'll also
use this method if you need a nonstandard build variant (see the
`Boost.Build documentation`_ for more details).

.. Admonition:: Boost.CMake

  There is also an experimental CMake build for boost, supported and distributed
  separately.  See the `Boost.CMake`_ wiki page for more information.

  .. _`Boost.CMake`:
       https://svn.boost.org/trac/boost/wiki/CMake

.. include:: detail/build-from-source-head.rst

For example, your session might look like this:

.. parsed-literal::

   $ cd ~/|boost_ver|
   $ b2 **--build-dir=**\ /tmp/build-boost **toolset=**\ gcc stage

That will build static and shared non-debug multi-threaded variants of the libraries. To build all variants, pass the additional option, “``--build-type=complete``”.

.. include:: detail/build-from-source-tail.rst

.. include:: detail/link-head.rst

There are two main ways to link to libraries:

A. You can specify the full path to each library:

   .. parsed-literal::

     $ c++ -I |root| example.cpp -o example **\\**
        **~/boost/stage/lib/libboost_regex-gcc34-mt-d-1_36.a**

B. You can separately specify a directory to search (with ``-L``\
   *directory*) and a library name to search for (with ``-l``\
   *library*, [#lowercase-l]_ dropping the filename's leading ``lib`` and trailing
   suffix (``.a`` in this case): 

   .. parsed-literal::

     $ c++ -I |root| example.cpp -o example **\\**
        **-L~/boost/stage/lib/ -lboost_regex-gcc34-mt-d-1_36**

   As you can see, this method is just as terse as method A for one
   library; it *really* pays off when you're using multiple
   libraries from the same directory.  Note, however, that if you
   use this method with a library that has both static (``.a``) and
   dynamic (``.so``) builds, the system may choose one
   automatically for you unless you pass a special option such as
   ``-static`` on the command line.

In both cases above, the bold text is what you'd add to `the
command lines we explored earlier`__.

__ `build a simple program using boost`_

Library Naming
--------------

.. include:: detail/library-naming.rst

.. include:: detail/test-head.rst

If you linked to a shared library, you may need to prepare some
platform-specific settings so that the system will be able to find
and load it when your program is run.  Most platforms have an
environment variable to which you can add the directory containing
the library.  On many platforms (Linux, FreeBSD) that variable is
``LD_LIBRARY_PATH``, but on MacOS it's ``DYLD_LIBRARY_PATH``, and
on Cygwin it's simply ``PATH``.  In most shells other than ``csh``
and ``tcsh``, you can adjust the variable as follows (again, don't
type the ``$``\ —that represents the shell prompt):

.. parsed-literal::

   **$** *VARIABLE_NAME*\ =\ *path/to/lib/directory*\ :${\ *VARIABLE_NAME*\ }
   **$** export *VARIABLE_NAME*

On ``csh`` and ``tcsh``, it's

.. parsed-literal::

   **$** setenv *VARIABLE_NAME* *path/to/lib/directory*\ :${\ *VARIABLE_NAME*\ }

Once the necessary variable (if any) is set, you can run your
program as follows:

.. parsed-literal::

   **$** *path*\ /\ *to*\ /\ *compiled*\ /\ example < *path*\ /\ *to*\ /\ jayne.txt

The program should respond with the email subject, “Will Success
Spoil Rock Hunter?”

.. include:: detail/conclusion.rst

------------------------------

.. [#packagers] If developers of Boost packages would like to work
   with us to make sure these instructions can be used with their
   packages, we'd be glad to help.  Please make your interest known
   to the `Boost developers' list`_.

   .. _Boost developers' list: http://www.boost.org/more/mailing_lists.htm#main

.. [#lowercase-l] That option is a dash followed by a lowercase “L”
   character, which looks very much like a numeral 1 in some fonts.

.. |build-type-complete| replace:: `` `` 

.. include:: detail/common-footnotes.rst
.. include:: detail/release-variables.rst
.. include:: detail/common-unix.rst
.. include:: detail/links.rst
