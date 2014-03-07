@echo off
rem Copyright Beman Dawes 2011
rem Distributed under the Boost Software License, Version 1.0.  See http://www.boost.org/LICENSE_1_0.txt 
if not %1$==$ goto usage_ok
echo Usage: 2release path-relative-to-boost-root [svn-options]
echo Path may be to file or directory
echo Options include --dry-run
echo WARNING: The current directory must be the directory in %BOOST_RELEASE%
echo          specified by the path-relative argument
goto done

:usage_ok
svn merge %2 %3 %4 %5 %6 https://svn.boost.org/svn/boost/branches/release/%1 ^
  https://svn.boost.org/svn/boost/trunk/%1

:done
