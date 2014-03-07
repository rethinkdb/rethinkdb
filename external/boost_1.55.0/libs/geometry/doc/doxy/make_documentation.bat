:: ===========================================================================
::  Copyright (c) 2010 Barend Gehrels, Amsterdam, the Netherlands.
:: 
::  Use, modification and distribution is subject to the Boost Software License,
::  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
::  http://www.boost.org/LICENSE_1_0.txt)9
:: ============================================================================

@echo off

doxygen

cd doxygen_output\html

for %%f in (*.html) do ..\..\doxygen_enhance.py %%f 

cd ..\..
