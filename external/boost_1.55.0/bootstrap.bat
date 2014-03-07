@ECHO OFF

REM Copyright (C) 2009 Vladimir Prus
REM
REM Distributed under the Boost Software License, Version 1.0.
REM (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

ECHO Building Boost.Build engine
if exist ".\tools\build\v2\engine\bin.ntx86\b2.exe" del tools\build\v2\engine\bin.ntx86\b2.exe
if exist ".\tools\build\v2\engine\bin.ntx86\bjam.exe" del tools\build\v2\engine\bin.ntx86\bjam.exe
if exist ".\tools\build\v2\engine\bin.ntx86_64\b2.exe" del tools\build\v2\engine\bin.ntx86_64\b2.exe
if exist ".\tools\build\v2\engine\bin.ntx86_64\bjam.exe" del tools\build\v2\engine\bin.ntx86_64\bjam.exe
pushd tools\build\v2\engine

call .\build.bat %* > ..\..\..\..\bootstrap.log
@ECHO OFF

popd

if exist ".\tools\build\v2\engine\bin.ntx86\bjam.exe" (
   copy .\tools\build\v2\engine\bin.ntx86\b2.exe . > nul
   copy .\tools\build\v2\engine\bin.ntx86\bjam.exe . > nul
   goto :bjam_built)

if exist ".\tools\build\v2\engine\bin.ntx86_64\bjam.exe" (
   copy .\tools\build\v2\engine\bin.ntx86_64\b2.exe . > nul
   copy .\tools\build\v2\engine\bin.ntx86_64\bjam.exe . > nul
   goto :bjam_built)

goto :bjam_failure

:bjam_built

REM Ideally, we should obtain the toolset that build.bat has
REM guessed. However, it uses setlocal at the start and does
REM export BOOST_JAM_TOOLSET, and I don't know how to do that
REM properly. Default to msvc for now.
set toolset=msvc

ECHO import option ; > project-config.jam
ECHO. >> project-config.jam
ECHO using %toolset% ; >> project-config.jam
ECHO. >> project-config.jam
ECHO option.set keep-going : false ; >> project-config.jam
ECHO. >> project-config.jam

ECHO.
ECHO Bootstrapping is done. To build, run:
ECHO.
ECHO     .\b2
ECHO.    
ECHO To adjust configuration, edit 'project-config.jam'.
ECHO Further information:
ECHO.
ECHO     - Command line help:
ECHO     .\b2 --help
ECHO.     
ECHO     - Getting started guide: 
ECHO     http://boost.org/more/getting_started/windows.html
ECHO.     
ECHO     - Boost.Build documentation:
ECHO     http://www.boost.org/boost-build2/doc/html/index.html

goto :end

:bjam_failure

ECHO.
ECHO Failed to build Boost.Build engine.
ECHO Please consult bootstrap.log for furter diagnostics.
ECHO.
ECHO You can try to obtain a prebuilt binary from
ECHO.
ECHO    http://sf.net/project/showfiles.php?group_id=7586^&package_id=72941
ECHO.
ECHO Also, you can file an issue at http://svn.boost.org 
ECHO Please attach bootstrap.log in that case.

goto :end

:end
