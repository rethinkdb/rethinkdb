.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

Header-Only Libraries
=====================

The first thing many people want to know is, “how do I build
Boost?”  The good news is that often, there's nothing to build.

.. admonition:: Nothing to Build?

  Most Boost libraries are **header-only**: they consist *entirely
  of header files* containing templates and inline functions, and
  require no separately-compiled library binaries or special
  treatment when linking.

.. .. _separate: 

The only Boost libraries that *must* be built separately are:

* Boost.Chrono_
* Boost.Context_
* Boost.Filesystem_
* Boost.GraphParallel_
* Boost.IOStreams_
* Boost.Locale_
* Boost.MPI_
* Boost.ProgramOptions_
* Boost.Python_ (see the `Boost.Python build documentation`__
  before building and installing it)
* Boost.Regex_
* Boost.Serialization_
* Boost.Signals_
* Boost.System_
* Boost.Thread_
* Boost.Timer_
* Boost.Wave_

__ ../../libs/python/doc/building.html

A few libraries have optional separately-compiled binaries:

* Boost.DateTime_ has a binary component that is only needed if
  you're using its ``to_string``\ /\ ``from_string`` or serialization
  features, or if you're targeting Visual C++ 6.x or Borland.

* Boost.Graph_ also has a binary component that is only needed if
  you intend to `parse GraphViz files`__.

* Boost.Math_ has binary components for the TR1 and C99
  cmath functions.

* Boost.Random_ has a binary component which is only needed if
  you're using ``random_device``.

* Boost.Test_ can be used in “header-only” or “separately compiled”
  mode, although **separate compilation is recommended for serious
  use**.

* Boost.Exception_ provides non-intrusive implementation of
  exception_ptr for 32-bit _MSC_VER==1310 and _MSC_VER==1400
  which requires a separately-compiled binary. This is enabled by
  #define BOOST_ENABLE_NON_INTRUSIVE_EXCEPTION_PTR.

__ ../../libs/graph/doc/read_graphviz.html
