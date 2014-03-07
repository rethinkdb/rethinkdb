.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

In order to choose the right binary for your build configuration
you need to know how Boost binaries are named.  Each library
filename is composed of a common sequence of elements that describe
how it was built.  For example,
``libboost_regex-vc71-mt-d-1_34.lib`` can be broken down into the
following elements:

``lib`` 
  *Prefix*: except on Microsoft Windows, every Boost library
  name begins with this string.  On Windows, only ordinary static
  libraries use the ``lib`` prefix; import libraries and DLLs do
  not. [#distinct]_

``boost_regex``
  *Library name*: all boost library filenames begin with ``boost_``.

``-vc71``
   *Toolset tag*: identifies the toolset_ and version used to build
   the binary.

``-mt``
   *Threading tag*: indicates that the library was
   built with multithreading support enabled.  Libraries built
   without multithreading support can be identified by the absence
   of ``-mt``.

``-d``
   *ABI tag*: encodes details that affect the library's
   interoperability with other compiled code.  For each such
   feature, a single letter is added to the tag:

     +-----+------------------------------------------------------------------------------+---------------------+
     |Key  |Use this library when:                                                        |Boost.Build option   |
     +=====+==============================================================================+=====================+
     |``s``|linking statically to the C++ standard library and compiler runtime support   |runtime-link=static  |
     |     |libraries.                                                                    |                     |
     +-----+------------------------------------------------------------------------------+---------------------+
     |``g``|using debug versions of the standard and runtime support libraries.           |runtime-debugging=on |
     +-----+------------------------------------------------------------------------------+---------------------+
     |``y``|using a special `debug build of Python`__.                                    |python-debugging=on  |
     +-----+------------------------------------------------------------------------------+---------------------+
     |``d``|building a debug version of your code. [#debug-abi]_                          |variant=debug        |
     +-----+------------------------------------------------------------------------------+---------------------+
     |``p``|using the STLPort standard library rather than the default one supplied with  |stdlib=stlport       |
     |     |your compiler.                                                                |                     |
     +-----+------------------------------------------------------------------------------+---------------------+

   For example, if you build a debug version of your code for use
   with debug versions of the static runtime library and the
   STLPort standard library in “native iostreams” mode,
   the tag would be: ``-sgdpn``.  If none of the above apply, the
   ABI tag is ommitted.

``-1_34``
  *Version tag*: the full Boost release number, with periods
  replaced by underscores. For example, version 1.31.1 would be
  tagged as "-1_31_1".

``.lib``
  *Extension*: determined according to the operating system's usual
  convention.  On most unix-style platforms the extensions are
  ``.a`` and ``.so`` for static libraries (archives) and shared
  libraries, respectively.  On Windows, ``.dll`` indicates a shared
  library and ``.lib`` indicates a
  static or import library.  Where supported by toolsets on unix
  variants, a full version extension is added (e.g. ".so.1.34") and
  a symbolic link to the library file, named without the trailing
  version number, will also be created.

.. .. _Boost.Build toolset names: toolset-name_

__ ../../libs/python/doc/building.html#python-debugging-builds

