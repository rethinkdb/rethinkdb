===========================================================================
 Copyright (c) 2007-2011 Barend Gehrels, Amsterdam, the Netherlands.
 Copyright (c) 2008-2011 Bruno Lalande, Paris, France.
 Copyright (c) 2009-2011 Mateusz Loskot, London, UK

 Use, modification and distribution is subject to the Boost Software License,
 Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
 http://www.boost.org/LICENSE_1_0.txt)
============================================================================

Procedure to create documentation using Doxygen, QuickBook and doxygen_xml2qbk

Note that doxygen_xml2qbk is a tool currently (slightly) specific to Boost.Geometry,
and that it translates from Doxygen-XML output to Quickbook (without xslt)

1) install Doxygen
2) install QuickBook using http://www.boost.org/doc/libs/1_45_0/doc/html/quickbook/install.html#quickbook.install.linux
3) unpack RapidXML, see src/docutils/tools/doxygen_xml2qbk/contrib/readme.txt
4) compile doxygen_xml2qbk, in src/docutils/tools/doxygen_xml2qbk
5) put binary somewhere, e.g. in /usr/local/bin/doxygen_xml2qbk
6) execute python file "make_qbk.py" (calling doxygen, doxygen_xml2qbk, bjam)

Folders in this folder:
concept: manually written documentation QBK files, on concept
doxy: folders and files needed for doxygen input and output
html: contains generated HTML files
other: older documentation (subject to update or deletion)
ref: manually written documentation QBK files, included from .hpp files
reference: generated documentation QBK files (by doxygen_xml2qbk)
	[note: this book cannot be called "generated" or something like that,
	because it is used in the final URL and we want to have "reference" in it]
src: examples used in documentation and tools (doxygen_xml2qbk)

Per new algorithm (e.g. foo), one should add:
1) in file boost/geometry/algorithms/foo.hpp, include a "\ingroup foo" in the doxygen comments
2) in file doc/doxy/doxygen_input/groups/groups.hpp, define the group "foo"
3) in file doc/make_qbk.py, include the algorithm "foo"
4) in file doc/reference.qbk, include the foo.qbk ([include generated/foo.qbk])
5) in file doc/quickref.xml, include a section on foo conform other sections
6) in file doc/src/docutils/tools/support_status/support_status.cpp include the algorithm (3 places) (optionally)
7) in file doc/reference/foo.qbk (to be created), include the support status and write other text, and include examples (optionally)
8) in file doc/imports.qbk, include the example foo.cpp (if any)
9) create file doc/src/examples/algorithm/foo.cpp (optional)


