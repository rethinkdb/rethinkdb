@echo off

rem  Copyright Beman Dawes 2011
rem  Distributed under to the Boost Software License, Version 1.0 
rem  See http://www.boost.org/LICENSE_1_0.txt

if not $%1==$ goto ok
:error
echo Usage: backup target-directory-path
goto done

:ok
mkdir %1\boost\filesystem 2>nul
mkdir %1\libs\filesystem 2>nul

set BOOST_CURRENT_ROOT=.
:loop
if exist %BOOST_CURRENT_ROOT%\boost-build.jam goto loopend
set BOOST_CURRENT_ROOT=..\%BOOST_CURRENT_ROOT%
goto loop
:loopend

xcopy /exclude:exclude.txt /y /d /k /r %BOOST_CURRENT_ROOT%\boost\filesystem.hpp %1\boost
xcopy /exclude:exclude.txt /y /d /k /s /r %BOOST_CURRENT_ROOT%\boost\filesystem %1\boost\filesystem
xcopy /exclude:exclude.txt /y /d /k /s /r %BOOST_CURRENT_ROOT%\libs\filesystem %1\libs\filesystem

:done
