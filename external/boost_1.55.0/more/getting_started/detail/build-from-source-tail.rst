.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

Building the special ``stage`` target places Boost
library binaries in the ``stage``\ |/|\ ``lib``\ |/| subdirectory of
the Boost tree.  To use a different directory pass the
``--stagedir=``\ *directory* option to ``b2``.

.. Note:: ``b2`` is case-sensitive; it is important that all the
   parts shown in **bold** type above be entirely lower-case.

For a description of other options you can pass when invoking
``b2``, type::

  b2 --help

In particular, to limit the amount of time spent building, you may
be interested in:

* reviewing the list of library names with ``--show-libraries``
* limiting which libraries get built with the ``--with-``\
  *library-name* or ``--without-``\ *library-name* options
* choosing a specific build variant by adding ``release`` or
  ``debug`` to the command line.

.. Note:: Boost.Build can produce a great deal of output, which can
     make it easy to miss problems.  If you want to make sure
     everything is went well, you might redirect the output into a
     file by appending “``>build.log 2>&1``” to your command line.

Expected Build Output
---------------------

During the process of building Boost libraries, you can expect to
see some messages printed on the console.  These may include

* Notices about Boost library configuration—for example, the Regex
  library outputs a message about ICU when built without Unicode
  support, and the Python library may be skipped without error (but
  with a notice) if you don't have Python installed.

* Messages from the build tool that report the number of targets
  that were built or skipped.  Don't be surprised if those numbers
  don't make any sense to you; there are many targets per library.

* Build action messages describing what the tool is doing, which
  look something like:

  .. parsed-literal::

    *toolset-name*.c++ *long*\ /\ *path*\ /\ *to*\ /\ *file*\ /\ *being*\ /\ *built*

* Compiler warnings.

In Case of Build Errors
-----------------------

The only error messages you see when building Boost—if any—should
be related to the IOStreams library's support of zip and bzip2
formats as described here__.  Install the relevant development
packages for libz and libbz2 if you need those features.  Other
errors when building Boost libraries are cause for concern.

__ ../../libs/iostreams/doc/installation.html

If it seems like the build system can't find your compiler and/or
linker, consider setting up a ``user-config.jam`` file as described
`here`__.  If that isn't your problem or the ``user-config.jam`` file
doesn't work for you, please address questions about configuring Boost
for your compiler to the `Boost.Build mailing list`_.

__ http://www.boost.org/boost-build2/doc/html/bbv2/advanced/configuration.html
