rem @echo off
rem Copyright Beman Dawes 2011
rem Distributed under the Boost Software License, Version 1.0.
rem See http://www.boost.org/LICENSE_1_0.txt 

pushd %BOOST_RELEASE%
svn up
call 2release Jamroot
call 2release index.html
pushd boost
call 2release boost/version.hpp
popd
pushd more
call 2release more
popd
popd
