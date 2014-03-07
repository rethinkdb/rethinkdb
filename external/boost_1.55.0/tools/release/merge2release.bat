rem @echo off
rem Copyright Beman Dawes 2010
rem Distributed under the Boost Software License, Version 1.0.  See http://www.boost.org/LICENSE_1_0.txt 
if not %1$==$ goto usage_ok
echo Usage: merge2release library-name [svn-options]
echo Options include --dry-run
goto done

:usage_ok
pushd %BOOST_RELEASE%
pushd boost
call 2release boost/%1.hpp %2 %3 %4 %5 %6
pushd %1
call 2release boost/%1 %2 %3 %4 %5 %6
popd
popd
pushd libs\%1
call 2release libs/%1 %2 %3 %4 %5 %6
popd
popd

:done
