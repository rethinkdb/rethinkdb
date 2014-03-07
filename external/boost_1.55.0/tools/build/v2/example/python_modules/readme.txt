Copyright 2006 Vladimir Prus
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)


This example shows how you can use Python modules from Boost.Build.

In order to do this, you need to build bjam with Python support, by running:

   ./build.sh --with-python=/usr

in the jam/src directory (replace /usr with the root of your Python
installation).

The integration between Python and bjam is very basic now, but enough to be
useful.
