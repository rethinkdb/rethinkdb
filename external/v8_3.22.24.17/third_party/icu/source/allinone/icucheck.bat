@echo off
REM  ********************************************************************
REM  * COPYRIGHT:
REM  * Copyright (c) 2010, International Business Machines Corporation
REM  * and others. All Rights Reserved.
REM  ********************************************************************

set ICU_ARCH=%1
set ICU_DBRL=%2

if "%1" == "" (
echo Usage: %0 "x86 or x64"  "Debug or Release"
exit /b 1
)

if "%2" == "" (
echo Usage: %0 %1 "Debug or Release"
exit /b 1
)

set ICU_OPATH=%PATH%

set ICU_ICUDIR=%~f0\..\..\..
set ICU_BINDIR=%ICU_ICUDIR%\bin
set PATH=%ICU_BINDIR%;%PATH%

echo testing ICU in %ICU_ICUDIR%  arch=%ICU_ARCH% type=%ICU_DBRL%
pushd %ICU_ICUDIR%

@rem factor these out
set ICUINFO_CMD=%ICU_ICUDIR%\source\tools\icuinfo\%ICU_ARCH%\%ICU_DBRL%\icuinfo.exe
set INTLTEST_CMD=%ICU_ICUDIR%\source\test\intltest\%ICU_ARCH%\%ICU_DBRL%\intltest.exe
set IOTEST_CMD=%ICU_ICUDIR%\source\test\iotest\%ICU_ARCH%\%ICU_DBRL%\iotest.exe
set CINTLTST_CMD=%ICU_ICUDIR%\source\test\cintltst\%ICU_ARCH%\%ICU_DBRL%\cintltst.exe
set LETEST_CMD=%ICU_ICUDIR%\source\test\letest\%ICU_ARCH%\%ICU_DBRL%\letest.exe

set ICUFAILED=
set ICURUN=
set ICUFAILCNT=0

@echo on

@set THT=icuinfo
@echo ==== %THT% =========================================================================
%ICUINFO_CMD% %ICUINFO_OPTS%

@IF NOT ERRORLEVEL 1 GOTO OK_%THT%
@set ICUFAILED=%ICUFAILED% %THT%
@set ICUFAILCNT=1
:OK_icuinfo
@set ICURUN=%ICURUN% %THT%

@set THT=intltest
@echo ==== %THT% =========================================================================
@cd %ICU_ICUDIR%\source\test\intltest
%INTLTEST_CMD% %INTLTEST_OPTS%

@IF NOT ERRORLEVEL 1 GOTO OK_%THT%
@set ICUFAILED=%ICUFAILED% %THT%
@set ICUFAILCNT=1
:OK_intltest
@set ICURUN=%ICURUN% %THT%

@set THT=iotest
@echo ==== %THT% =========================================================================
@cd %ICU_ICUDIR%\source\test\iotest
%IOTEST_CMD% %IOTEST_OPTS%

@IF NOT ERRORLEVEL 1 GOTO OK_%THT%
@set ICUFAILED=%ICUFAILED% %THT%
@set ICUFAILCNT=1
:OK_IOTEST
@set ICURUN=%ICURUN% %THT%

@set THT=cintltst
@echo ==== %THT% =========================================================================
@cd %ICU_ICUDIR%\source\test\cintltst
%CINTLTST_CMD% %CINTLTST_OPTS%

@IF NOT ERRORLEVEL 1 GOTO OK_%THT%
@set ICUFAILED=%ICUFAILED% %THT%
@set ICUFAILCNT=1
:OK_cintltst
@set ICURUN=%ICURUN% %THT%

@set THT=letest
@echo ==== %THT% =========================================================================
@cd %ICU_ICUDIR%\source\test\letest
%LETST_CMD% %LETEST_OPTS%

@IF NOT ERRORLEVEL 1 GOTO OK_%THT%
@set ICUFAILED=%ICUFAILED% %THT%
@set ICUFAILCNT=1
:OK_letest
@set ICURUN=%ICURUN% %THT%

@echo off

REM clean up
set PATH=%ICU_OPATH%
REM unset ICU_OPATH
popd

@REM done

echo -
echo -
echo -
echo ============================================================
echo Summary:
echo -
echo Tests Run    : %ICURUN%

if %ICUFAILCNT% == 0 (
	echo " - All Passed!"
	exit /b 0
)
echo Failing Tests: %ICUFAILED%
echo -
echo FAILED!

exit /b 1