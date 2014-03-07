@echo off
rem Copyright Beman Dawes 2012
rem Distributed under the Boost Software License, Version 1.0.
del tr2.html 2>nul
mmp TARGET=TR2 source.html tr2.html
del reference.html 2>nul
mmp TARGET=BOOST source.html reference.html
echo run "hoist" to hoist reference.html to doc directory
