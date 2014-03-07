Copyright 2003 Vladimir Prus
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)


This example shows how it is possible to use GNU gettext utilities with
Boost.Build.

A simple translation file is compiled and installed as message catalog for
russian. The main application explicitly switches to russian locale and outputs
the translation of "hello".

To test:

   bjam
   bin/gcc/debug/main

To test even more:

   - add more localized strings to "main.cpp"
   - run "bjam update-russian"
   - edit "russian.po"
   - run bjam
   - run "main"
