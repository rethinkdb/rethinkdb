@echo off
rem Copyright Beman Dawes 2011
rem Distributed under the Boost Software License, Version 1.0.  See http://www.boost.org/LICENSE_1_0.txt 
if not %1$==$ goto usage_ok
echo Usage: unmerged_whatever path-from-root [svn-options]
echo Options include --summarize to show paths only. i.e. suppresses line-by-line diffs
goto done

:usage_ok
svn diff %2 %3 %4 %5 %6 http://svn.boost.org/svn/boost/branches/release/%1 ^
  http://svn.boost.org/svn/boost/trunk/%1

:done
