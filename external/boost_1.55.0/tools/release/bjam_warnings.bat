@echo off
rem  Scan bjam/b2 log file for compile warnings

rem  Copyright 2011 Beman Dawes

rem  Distributed under the Boost Software License, Version 1.0.
rem  See http://www.boost.org/LICENSE_1_0.txt

if not %1$==$ goto usage_ok
echo Usage: bjam_warnings log-path
goto done

:usage_ok

grep -i "warning" %1 | grep -E "boost|libs" | sort | uniq

:done
