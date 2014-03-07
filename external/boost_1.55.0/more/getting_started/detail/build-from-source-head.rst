.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

Install Boost.Build
...................

Boost.Build_ is a text-based system for developing, testing, and
installing software. First, you'll need to build and
install it. To do this:

1. Go to the directory ``tools``\ |/|\ ``build``\ |/|\ ``v2``\ |/|.
2. Run |bootstrap|
3. Run ``b2 install --prefix=``\ *PREFIX* where *PREFIX* is
   the directory where you want Boost.Build to be installed
4. Add *PREFIX*\ |/|\ ``bin`` to your PATH environment variable.

.. _Boost.Build: ../../tools/build/index.html
.. _Boost.Build documentation: Boost.Build_

.. _toolset:
.. _toolset-name:

Identify Your Toolset
.....................

First, find the toolset corresponding to your compiler in the
following table (an up-to-date list is always available `in the
Boost.Build documentation`__).

__ http://www.boost.org/boost-build2/doc/html/bbv2/reference/tools.html

.. Note:: If you previously chose a toolset for the purposes of
  `building b2`_, you should assume it won't work and instead
  choose newly from the table below.

.. _building b2: ../../doc/html/bbv2/installation.html

+-----------+--------------------+-----------------------------+
|Toolset    |Vendor              |Notes                        |
|Name       |                    |                             |
+===========+====================+=============================+
|``acc``    |Hewlett Packard     |Only very recent versions are|
|           |                    |known to work well with Boost|
+-----------+--------------------+-----------------------------+
|``borland``|Borland             |                             |
+-----------+--------------------+-----------------------------+
|``como``   |Comeau Computing    |Using this toolset may       |
|           |                    |require configuring__ another|
|           |                    |toolset to act as its backend|
+-----------+--------------------+-----------------------------+
|``darwin`` |Apple Computer      |Apple's version of the GCC   |
|           |                    |toolchain with support for   |
|           |                    |Darwin and MacOS X features  |
|           |                    |such as frameworks.          |
+-----------+--------------------+-----------------------------+
|``gcc``    |The Gnu Project     |Includes support for Cygwin  |
|           |                    |and MinGW compilers.         |
+-----------+--------------------+-----------------------------+
|``hp_cxx`` |Hewlett Packard     |Targeted at the Tru64        |
|           |                    |operating system.            |
+-----------+--------------------+-----------------------------+
|``intel``  |Intel               |                             |
+-----------+--------------------+-----------------------------+
|``msvc``   |Microsoft           |                             |
+-----------+--------------------+-----------------------------+
|``sun``    |Sun                 |Only very recent versions are|
|           |                    |known to work well with      |
|           |                    |Boost.                       |
+-----------+--------------------+-----------------------------+
|``vacpp``  |IBM                 |The VisualAge C++ compiler.  |
+-----------+--------------------+-----------------------------+

__ Boost.Build_

If you have multiple versions of a particular compiler installed,
you can append the version number to the toolset name, preceded by
a hyphen, e.g. ``intel-9.0`` or
``borland-5.4.3``. |windows-version-name-caveat|


.. _build directory:
.. _build-directory:

Select a Build Directory
........................

Boost.Build_ will place all intermediate files it generates while
building into the **build directory**.  If your Boost root
directory is writable, this step isn't strictly necessary: by
default Boost.Build will create a ``bin.v2/`` subdirectory for that
purpose in your current working directory.

Invoke ``b2``
...............

.. |build-directory| replace:: *build-directory*
.. |toolset-name| replace:: *toolset-name*

Change your current directory to the Boost root directory and
invoke ``b2`` as follows:

.. parsed-literal::

  b2 **--build-dir=**\ |build-directory|_ **toolset=**\ |toolset-name|_ |build-type-complete| stage

For a complete description of these and other invocation options,
please see the `Boost.Build documentation`__.

__ http://www.boost.org/boost-build2/doc/html/bbv2/advanced/invocation.html

