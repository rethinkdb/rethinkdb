Copyright 2005 Vladimir Prus 
Distributed under the Boost Software License, Version 1.0. 
(See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt) 


This directory contains Boost.Build examples for the Qt library
(http://www.trolltech.com/products/qt/index.html).

The current examples are:
  1. Basic setup -- application with several sources and moccable header.
  2. Using of .ui source file.
  3. Running .cpp files via the moc tool.

For convenience, there are examples both for 3.* and 4.* version of Qt, they are
mostly identical and differ only in source code.

All examples assumes that you just installed Boost.Build and that QTDIR
environment variables is set (typical values can be /usr/share/qt3 and
/usr/share/qt4). After adding "using qt ..." to your user-config.jam, you would
have to remove "using qt ; " statements from example Jamroot files.
