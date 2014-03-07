@ECHO OFF

REM ~ Copyright 2002-2007 Rene Rivera.
REM ~ Distributed under the Boost Software License, Version 1.0.
REM ~ (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

setlocal
goto Start


:Set_Error
color 00
goto :eof


:Clear_Error
ver >nul
goto :eof


:Error_Print
REM Output an error message and set the errorlevel to indicate failure.
setlocal
ECHO ###
ECHO ### %1
ECHO ###
ECHO ### You can specify the toolset as the argument, i.e.:
ECHO ###     .\build.bat msvc
ECHO ###
ECHO ### Toolsets supported by this script are: borland, como, gcc, gcc-nocygwin,
ECHO ###     intel-win32, metrowerks, mingw, msvc, vc7, vc8, vc9, vc10, vc11, vc12
ECHO ###
call :Set_Error
endlocal
goto :eof


:Test_Path
REM Tests for the given file(executable) presence in the directories in the PATH
REM environment variable. Additionaly sets FOUND_PATH to the path of the
REM found file.
call :Clear_Error
setlocal
set test=%~$PATH:1
endlocal
if not errorlevel 1 set FOUND_PATH=%~dp$PATH:1
goto :eof


:Test_Option
REM Tests whether the given string is in the form of an option: "--*"
call :Clear_Error
setlocal
set test=%1
if not defined test (
    call :Set_Error
    goto Test_Option_End
)
set test=###%test%###
set test=%test:"###=%
set test=%test:###"=%
set test=%test:###=%
if not "-" == "%test:~1,1%" call :Set_Error
:Test_Option_End
endlocal
goto :eof


:Test_Empty
REM Tests whether the given string is not empty
call :Clear_Error
setlocal
set test=%1
if not defined test (
    call :Clear_Error
    goto Test_Empty_End
)
set test=###%test%###
set test=%test:"###=%
set test=%test:###"=%
set test=%test:###=%
if not "" == "%test%" call :Set_Error
:Test_Empty_End
endlocal
goto :eof


:Call_If_Exists
if EXIST %1 call %*
goto :eof


:Guess_Toolset
REM Try and guess the toolset to bootstrap the build with...
REM Sets BOOST_JAM_TOOLSET to the first found toolset.
REM May also set BOOST_JAM_TOOLSET_ROOT to the
REM location of the found toolset.

call :Clear_Error
call :Test_Empty %ProgramFiles%
if not errorlevel 1 set ProgramFiles=C:\Program Files

call :Clear_Error
if NOT "_%VS120COMNTOOLS%_" == "__" (
    set "BOOST_JAM_TOOLSET=vc12"
    set "BOOST_JAM_TOOLSET_ROOT=%VS120COMNTOOLS%..\..\VC\"
    goto :eof)
call :Clear_Error
if EXIST "%ProgramFiles%\Microsoft Visual Studio 12.0\VC\VCVARSALL.BAT" (
    set "BOOST_JAM_TOOLSET=vc12"
    set "BOOST_JAM_TOOLSET_ROOT=%ProgramFiles%\Microsoft Visual Studio 12.0\VC\"
    goto :eof)
call :Clear_Error
if NOT "_%VS110COMNTOOLS%_" == "__" (
    set "BOOST_JAM_TOOLSET=vc11"
    set "BOOST_JAM_TOOLSET_ROOT=%VS110COMNTOOLS%..\..\VC\"
    goto :eof)
call :Clear_Error
if EXIST "%ProgramFiles%\Microsoft Visual Studio 11.0\VC\VCVARSALL.BAT" (
    set "BOOST_JAM_TOOLSET=vc11"
    set "BOOST_JAM_TOOLSET_ROOT=%ProgramFiles%\Microsoft Visual Studio 11.0\VC\"
    goto :eof)
call :Clear_Error
if NOT "_%VS100COMNTOOLS%_" == "__" (
    set "BOOST_JAM_TOOLSET=vc10"
    set "BOOST_JAM_TOOLSET_ROOT=%VS100COMNTOOLS%..\..\VC\"
    goto :eof)
call :Clear_Error
if EXIST "%ProgramFiles%\Microsoft Visual Studio 10.0\VC\VCVARSALL.BAT" (
    set "BOOST_JAM_TOOLSET=vc10"
    set "BOOST_JAM_TOOLSET_ROOT=%ProgramFiles%\Microsoft Visual Studio 10.0\VC\"
    goto :eof)
call :Clear_Error
if NOT "_%VS90COMNTOOLS%_" == "__" (
    set "BOOST_JAM_TOOLSET=vc9"
    set "BOOST_JAM_TOOLSET_ROOT=%VS90COMNTOOLS%..\..\VC\"
    goto :eof)
call :Clear_Error
if EXIST "%ProgramFiles%\Microsoft Visual Studio 9.0\VC\VCVARSALL.BAT" (
    set "BOOST_JAM_TOOLSET=vc9"
    set "BOOST_JAM_TOOLSET_ROOT=%ProgramFiles%\Microsoft Visual Studio 9.0\VC\"
    goto :eof)
call :Clear_Error
if NOT "_%VS80COMNTOOLS%_" == "__" (
    set "BOOST_JAM_TOOLSET=vc8"
    set "BOOST_JAM_TOOLSET_ROOT=%VS80COMNTOOLS%..\..\VC\"
    goto :eof)
call :Clear_Error
if EXIST "%ProgramFiles%\Microsoft Visual Studio 8\VC\VCVARSALL.BAT" (
    set "BOOST_JAM_TOOLSET=vc8"
    set "BOOST_JAM_TOOLSET_ROOT=%ProgramFiles%\Microsoft Visual Studio 8\VC\"
    goto :eof)
call :Clear_Error
if NOT "_%VS71COMNTOOLS%_" == "__" (
    set "BOOST_JAM_TOOLSET=vc7"
    set "BOOST_JAM_TOOLSET_ROOT=%VS71COMNTOOLS%\..\..\VC7\"
    goto :eof)
call :Clear_Error
if NOT "_%VCINSTALLDIR%_" == "__" (
    REM %VCINSTALLDIR% is also set for VC9 (and probably VC8)
    set "BOOST_JAM_TOOLSET=vc7"
    set "BOOST_JAM_TOOLSET_ROOT=%VCINSTALLDIR%\VC7\"
    goto :eof)
call :Clear_Error
if EXIST "%ProgramFiles%\Microsoft Visual Studio .NET 2003\VC7\bin\VCVARS32.BAT" (
    set "BOOST_JAM_TOOLSET=vc7"
    set "BOOST_JAM_TOOLSET_ROOT=%ProgramFiles%\Microsoft Visual Studio .NET 2003\VC7\"
    goto :eof)
call :Clear_Error
if EXIST "%ProgramFiles%\Microsoft Visual Studio .NET\VC7\bin\VCVARS32.BAT" (
    set "BOOST_JAM_TOOLSET=vc7"
    set "BOOST_JAM_TOOLSET_ROOT=%ProgramFiles%\Microsoft Visual Studio .NET\VC7\"
    goto :eof)
call :Clear_Error
if NOT "_%MSVCDir%_" == "__" (
    set "BOOST_JAM_TOOLSET=msvc"
    set "BOOST_JAM_TOOLSET_ROOT=%MSVCDir%\"
    goto :eof)
call :Clear_Error
if EXIST "%ProgramFiles%\Microsoft Visual Studio\VC98\bin\VCVARS32.BAT" (
    set "BOOST_JAM_TOOLSET=msvc"
    set "BOOST_JAM_TOOLSET_ROOT=%ProgramFiles%\Microsoft Visual Studio\VC98\"
    goto :eof)
call :Clear_Error
if EXIST "%ProgramFiles%\Microsoft Visual C++\VC98\bin\VCVARS32.BAT" (
    set "BOOST_JAM_TOOLSET=msvc"
    set "BOOST_JAM_TOOLSET_ROOT=%ProgramFiles%\Microsoft Visual C++\VC98\"
    goto :eof)
call :Clear_Error
call :Test_Path cl.exe
if not errorlevel 1 (
    set "BOOST_JAM_TOOLSET=msvc"
    set "BOOST_JAM_TOOLSET_ROOT=%FOUND_PATH%..\"
    goto :eof)
call :Clear_Error
call :Test_Path vcvars32.bat
if not errorlevel 1 (
    set "BOOST_JAM_TOOLSET=msvc"
    call "%FOUND_PATH%VCVARS32.BAT"
    set "BOOST_JAM_TOOLSET_ROOT=%MSVCDir%\"
    goto :eof)
call :Clear_Error
if EXIST "C:\Borland\BCC55\Bin\bcc32.exe" (
    set "BOOST_JAM_TOOLSET=borland"
    set "BOOST_JAM_TOOLSET_ROOT=C:\Borland\BCC55\"
    goto :eof)
call :Clear_Error
call :Test_Path bcc32.exe
if not errorlevel 1 (
    set "BOOST_JAM_TOOLSET=borland"
    set "BOOST_JAM_TOOLSET_ROOT=%FOUND_PATH%..\"
    goto :eof)
call :Clear_Error
call :Test_Path icl.exe
if not errorlevel 1 (
    set "BOOST_JAM_TOOLSET=intel-win32"
    set "BOOST_JAM_TOOLSET_ROOT=%FOUND_PATH%..\"
    goto :eof)
call :Clear_Error
if EXIST "C:\MinGW\bin\gcc.exe" (
    set "BOOST_JAM_TOOLSET=mingw"
    set "BOOST_JAM_TOOLSET_ROOT=C:\MinGW\"
    goto :eof)
call :Clear_Error
if NOT "_%CWFolder%_" == "__" (
    set "BOOST_JAM_TOOLSET=metrowerks"
    set "BOOST_JAM_TOOLSET_ROOT=%CWFolder%\"
    goto :eof )
call :Clear_Error
call :Test_Path mwcc.exe
if not errorlevel 1 (
    set "BOOST_JAM_TOOLSET=metrowerks"
    set "BOOST_JAM_TOOLSET_ROOT=%FOUND_PATH%..\..\"
    goto :eof)
call :Clear_Error
call :Error_Print "Could not find a suitable toolset."
goto :eof


:Guess_Yacc
REM Tries to find bison or yacc in common places so we can build the grammar.
call :Clear_Error
call :Test_Path yacc.exe
if not errorlevel 1 (
    set "YACC=yacc -d"
    goto :eof)
call :Clear_Error
call :Test_Path bison.exe
if not errorlevel 1 (
    set "YACC=bison -d --yacc"
    goto :eof)
call :Clear_Error
if EXIST "C:\Program Files\GnuWin32\bin\bison.exe" (
    set "YACC=C:\Program Files\GnuWin32\bin\bison.exe" -d --yacc
    goto :eof)
call :Clear_Error
call :Error_Print "Could not find Yacc to build the Jam grammar."
goto :eof


:Start
set BOOST_JAM_TOOLSET=
set BOOST_JAM_ARGS=

REM If no arguments guess the toolset;
REM or if first argument is an option guess the toolset;
REM otherwise the argument is the toolset to use.
call :Clear_Error
call :Test_Empty %1
if not errorlevel 1 (
    call :Guess_Toolset
    if not errorlevel 1 ( goto Setup_Toolset ) else ( goto Finish )
)

call :Clear_Error
call :Test_Option %1
if not errorlevel 1 (
    call :Guess_Toolset
    if not errorlevel 1 ( goto Setup_Toolset ) else ( goto Finish )
)

call :Clear_Error
set BOOST_JAM_TOOLSET=%1
shift
goto Setup_Toolset


:Setup_Toolset
REM Setup the toolset command and options. This bit of code
REM needs to be flexible enough to handle both when
REM the toolset was guessed at and found, or when the toolset
REM was indicated in the command arguments.
REM NOTE: The strange multiple "if ?? == _toolset_" tests are that way
REM because in BAT variables are subsituted only once during a single
REM command. A complete "if ... ( commands ) else ( commands )"
REM is a single command, even though it's in multiple lines here.
:Setup_Args
call :Clear_Error
call :Test_Empty %1
if not errorlevel 1 goto Config_Toolset
call :Clear_Error
call :Test_Option %1
if errorlevel 1 (
    set BOOST_JAM_ARGS=%BOOST_JAM_ARGS% %1
    shift
    goto Setup_Args
)
:Config_Toolset
if NOT "_%BOOST_JAM_TOOLSET%_" == "_metrowerks_" goto Skip_METROWERKS
if NOT "_%CWFolder%_" == "__" (
    set "BOOST_JAM_TOOLSET_ROOT=%CWFolder%\"
    )
set "PATH=%BOOST_JAM_TOOLSET_ROOT%Other Metrowerks Tools\Command Line Tools;%PATH%"
set "BOOST_JAM_CC=mwcc -runtime ss -cwd include -DNT -lkernel32.lib -ladvapi32.lib -luser32.lib"
set "BOOST_JAM_OPT_JAM=-o bootstrap\jam0.exe"
set "BOOST_JAM_OPT_MKJAMBASE=-o bootstrap\mkjambase0.exe"
set "BOOST_JAM_OPT_YYACC=-o bootstrap\yyacc0.exe"
set "_known_=1"
:Skip_METROWERKS
if NOT "_%BOOST_JAM_TOOLSET%_" == "_msvc_" goto Skip_MSVC
if NOT "_%MSVCDir%_" == "__" (
    set "BOOST_JAM_TOOLSET_ROOT=%MSVCDir%\"
    )
call :Call_If_Exists "%BOOST_JAM_TOOLSET_ROOT%bin\VCVARS32.BAT"
if not "_%BOOST_JAM_TOOLSET_ROOT%_" == "__" (
    set "PATH=%BOOST_JAM_TOOLSET_ROOT%bin;%PATH%"
    )
set "BOOST_JAM_CC=cl /nologo /GZ /Zi /MLd /Fobootstrap/ /Fdbootstrap/ -DNT -DYYDEBUG kernel32.lib advapi32.lib user32.lib"
set "BOOST_JAM_OPT_JAM=/Febootstrap\jam0"
set "BOOST_JAM_OPT_MKJAMBASE=/Febootstrap\mkjambase0"
set "BOOST_JAM_OPT_YYACC=/Febootstrap\yyacc0"
set "_known_=1"
:Skip_MSVC
if NOT "_%BOOST_JAM_TOOLSET%_" == "_vc7_" goto Skip_VC7
if NOT "_%VS71COMNTOOLS%_" == "__" (
    set "BOOST_JAM_TOOLSET_ROOT=%VS71COMNTOOLS%..\..\VC7\"
    )
if "_%VCINSTALLDIR%_" == "__" call :Call_If_Exists "%BOOST_JAM_TOOLSET_ROOT%bin\VCVARS32.BAT"
if NOT "_%BOOST_JAM_TOOLSET_ROOT%_" == "__" (
    if "_%VCINSTALLDIR%_" == "__" (
        set "PATH=%BOOST_JAM_TOOLSET_ROOT%bin;%PATH%"
        ) )
set "BOOST_JAM_CC=cl /nologo /GZ /Zi /MLd /Fobootstrap/ /Fdbootstrap/ -DNT -DYYDEBUG kernel32.lib advapi32.lib user32.lib"
set "BOOST_JAM_OPT_JAM=/Febootstrap\jam0"
set "BOOST_JAM_OPT_MKJAMBASE=/Febootstrap\mkjambase0"
set "BOOST_JAM_OPT_YYACC=/Febootstrap\yyacc0"
set "_known_=1"
:Skip_VC7
if NOT "_%BOOST_JAM_TOOLSET%_" == "_vc8_" goto Skip_VC8
if NOT "_%VS80COMNTOOLS%_" == "__" (
    set "BOOST_JAM_TOOLSET_ROOT=%VS80COMNTOOLS%..\..\VC\"
    )
if "_%VCINSTALLDIR%_" == "__" call :Call_If_Exists "%BOOST_JAM_TOOLSET_ROOT%VCVARSALL.BAT" %BOOST_JAM_ARGS%
if NOT "_%BOOST_JAM_TOOLSET_ROOT%_" == "__" (
    if "_%VCINSTALLDIR%_" == "__" (
        set "PATH=%BOOST_JAM_TOOLSET_ROOT%bin;%PATH%"
        ) )
set "BOOST_JAM_CC=cl /nologo /RTC1 /Zi /MTd /Fobootstrap/ /Fdbootstrap/ -DNT -DYYDEBUG -wd4996 kernel32.lib advapi32.lib user32.lib"
set "BOOST_JAM_OPT_JAM=/Febootstrap\jam0"
set "BOOST_JAM_OPT_MKJAMBASE=/Febootstrap\mkjambase0"
set "BOOST_JAM_OPT_YYACC=/Febootstrap\yyacc0"
set "_known_=1"
:Skip_VC8
if NOT "_%BOOST_JAM_TOOLSET%_" == "_vc9_" goto Skip_VC9
if NOT "_%VS90COMNTOOLS%_" == "__" (
    set "BOOST_JAM_TOOLSET_ROOT=%VS90COMNTOOLS%..\..\VC\"
    )
if "_%VCINSTALLDIR%_" == "__" call :Call_If_Exists "%BOOST_JAM_TOOLSET_ROOT%VCVARSALL.BAT" %BOOST_JAM_ARGS%
if NOT "_%BOOST_JAM_TOOLSET_ROOT%_" == "__" (
    if "_%VCINSTALLDIR%_" == "__" (
        set "PATH=%BOOST_JAM_TOOLSET_ROOT%bin;%PATH%"
        ) )
set "BOOST_JAM_CC=cl /nologo /RTC1 /Zi /MTd /Fobootstrap/ /Fdbootstrap/ -DNT -DYYDEBUG -wd4996 kernel32.lib advapi32.lib user32.lib"
set "BOOST_JAM_OPT_JAM=/Febootstrap\jam0"
set "BOOST_JAM_OPT_MKJAMBASE=/Febootstrap\mkjambase0"
set "BOOST_JAM_OPT_YYACC=/Febootstrap\yyacc0"
set "_known_=1"
:Skip_VC9
if NOT "_%BOOST_JAM_TOOLSET%_" == "_vc10_" goto Skip_VC10
if NOT "_%VS100COMNTOOLS%_" == "__" (
    set "BOOST_JAM_TOOLSET_ROOT=%VS100COMNTOOLS%..\..\VC\"
    )
if "_%VCINSTALLDIR%_" == "__" call :Call_If_Exists "%BOOST_JAM_TOOLSET_ROOT%VCVARSALL.BAT" %BOOST_JAM_ARGS%
if NOT "_%BOOST_JAM_TOOLSET_ROOT%_" == "__" (
    if "_%VCINSTALLDIR%_" == "__" (
        set "PATH=%BOOST_JAM_TOOLSET_ROOT%bin;%PATH%"
        ) )
set "BOOST_JAM_CC=cl /nologo /RTC1 /Zi /MTd /Fobootstrap/ /Fdbootstrap/ -DNT -DYYDEBUG -wd4996 kernel32.lib advapi32.lib user32.lib"
set "BOOST_JAM_OPT_JAM=/Febootstrap\jam0"
set "BOOST_JAM_OPT_MKJAMBASE=/Febootstrap\mkjambase0"
set "BOOST_JAM_OPT_YYACC=/Febootstrap\yyacc0"
set "_known_=1"
:Skip_VC10
if NOT "_%BOOST_JAM_TOOLSET%_" == "_vc11_" goto Skip_VC11
if NOT "_%VS110COMNTOOLS%_" == "__" (
    set "BOOST_JAM_TOOLSET_ROOT=%VS110COMNTOOLS%..\..\VC\"
    )
if "_%VCINSTALLDIR%_" == "__" call :Call_If_Exists "%BOOST_JAM_TOOLSET_ROOT%VCVARSALL.BAT" %BOOST_JAM_ARGS%
if NOT "_%BOOST_JAM_TOOLSET_ROOT%_" == "__" (
    if "_%VCINSTALLDIR%_" == "__" (
        set "PATH=%BOOST_JAM_TOOLSET_ROOT%bin;%PATH%"
        ) )
set "BOOST_JAM_CC=cl /nologo /RTC1 /Zi /MTd /Fobootstrap/ /Fdbootstrap/ -DNT -DYYDEBUG -wd4996 kernel32.lib advapi32.lib user32.lib"
set "BOOST_JAM_OPT_JAM=/Febootstrap\jam0"
set "BOOST_JAM_OPT_MKJAMBASE=/Febootstrap\mkjambase0"
set "BOOST_JAM_OPT_YYACC=/Febootstrap\yyacc0"
set "_known_=1"
:Skip_VC11
if NOT "_%BOOST_JAM_TOOLSET%_" == "_vc12_" goto Skip_VC12
if NOT "_%VS120COMNTOOLS%_" == "__" (
    set "BOOST_JAM_TOOLSET_ROOT=%VS120COMNTOOLS%..\..\VC\"
    )
if "_%VCINSTALLDIR%_" == "__" call :Call_If_Exists "%BOOST_JAM_TOOLSET_ROOT%VCVARSALL.BAT" %BOOST_JAM_ARGS%
if NOT "_%BOOST_JAM_TOOLSET_ROOT%_" == "__" (
    if "_%VCINSTALLDIR%_" == "__" (
        set "PATH=%BOOST_JAM_TOOLSET_ROOT%bin;%PATH%"
        ) )
set "BOOST_JAM_CC=cl /nologo /RTC1 /Zi /MTd /Fobootstrap/ /Fdbootstrap/ -DNT -DYYDEBUG -wd4996 kernel32.lib advapi32.lib user32.lib"
set "BOOST_JAM_OPT_JAM=/Febootstrap\jam0"
set "BOOST_JAM_OPT_MKJAMBASE=/Febootstrap\mkjambase0"
set "BOOST_JAM_OPT_YYACC=/Febootstrap\yyacc0"
set "_known_=1"
:Skip_VC12
if NOT "_%BOOST_JAM_TOOLSET%_" == "_borland_" goto Skip_BORLAND
if "_%BOOST_JAM_TOOLSET_ROOT%_" == "__" (
    call :Test_Path bcc32.exe )
if "_%BOOST_JAM_TOOLSET_ROOT%_" == "__" (
    if not errorlevel 1 (
        set "BOOST_JAM_TOOLSET_ROOT=%FOUND_PATH%..\"
        ) )
if not "_%BOOST_JAM_TOOLSET_ROOT%_" == "__" (
    set "PATH=%BOOST_JAM_TOOLSET_ROOT%Bin;%PATH%"
    )
set "BOOST_JAM_CC=bcc32 -WC -w- -q -I%BOOST_JAM_TOOLSET_ROOT%Include -L%BOOST_JAM_TOOLSET_ROOT%Lib /DNT -nbootstrap"
set "BOOST_JAM_OPT_JAM=-ejam0"
set "BOOST_JAM_OPT_MKJAMBASE=-emkjambasejam0"
set "BOOST_JAM_OPT_YYACC=-eyyacc0"
set "_known_=1"
:Skip_BORLAND
if NOT "_%BOOST_JAM_TOOLSET%_" == "_como_" goto Skip_COMO
set "BOOST_JAM_CC=como -DNT"
set "BOOST_JAM_OPT_JAM=-o bootstrap\jam0.exe"
set "BOOST_JAM_OPT_MKJAMBASE=-o bootstrap\mkjambase0.exe"
set "BOOST_JAM_OPT_YYACC=-o bootstrap\yyacc0.exe"
set "_known_=1"
:Skip_COMO
if NOT "_%BOOST_JAM_TOOLSET%_" == "_gcc_" goto Skip_GCC
set "BOOST_JAM_CC=gcc -DNT"
set "BOOST_JAM_OPT_JAM=-o bootstrap\jam0.exe"
set "BOOST_JAM_OPT_MKJAMBASE=-o bootstrap\mkjambase0.exe"
set "BOOST_JAM_OPT_YYACC=-o bootstrap\yyacc0.exe"
set "_known_=1"
:Skip_GCC
if NOT "_%BOOST_JAM_TOOLSET%_" == "_gcc-nocygwin_" goto Skip_GCC_NOCYGWIN
set "BOOST_JAM_CC=gcc -DNT -mno-cygwin"
set "BOOST_JAM_OPT_JAM=-o bootstrap\jam0.exe"
set "BOOST_JAM_OPT_MKJAMBASE=-o bootstrap\mkjambase0.exe"
set "BOOST_JAM_OPT_YYACC=-o bootstrap\yyacc0.exe"
set "_known_=1"
:Skip_GCC_NOCYGWIN
if NOT "_%BOOST_JAM_TOOLSET%_" == "_intel-win32_" goto Skip_INTEL_WIN32
set "BOOST_JAM_CC=icl -DNT /nologo kernel32.lib advapi32.lib user32.lib"
set "BOOST_JAM_OPT_JAM=/Febootstrap\jam0"
set "BOOST_JAM_OPT_MKJAMBASE=/Febootstrap\mkjambase0"
set "BOOST_JAM_OPT_YYACC=/Febootstrap\yyacc0"
set "_known_=1"
:Skip_INTEL_WIN32
if NOT "_%BOOST_JAM_TOOLSET%_" == "_mingw_" goto Skip_MINGW
if not "_%BOOST_JAM_TOOLSET_ROOT%_" == "__" (
    set "PATH=%BOOST_JAM_TOOLSET_ROOT%bin;%PATH%"
    )
set "BOOST_JAM_CC=gcc -DNT"
set "BOOST_JAM_OPT_JAM=-o bootstrap\jam0.exe"
set "BOOST_JAM_OPT_MKJAMBASE=-o bootstrap\mkjambase0.exe"
set "BOOST_JAM_OPT_YYACC=-o bootstrap\yyacc0.exe"
set "_known_=1"
:Skip_MINGW
call :Clear_Error
if "_%_known_%_" == "__" (
    call :Error_Print "Unknown toolset: %BOOST_JAM_TOOLSET%"
)
if errorlevel 1 goto Finish

echo ###
echo ### Using '%BOOST_JAM_TOOLSET%' toolset.
echo ###

set YYACC_SOURCES=yyacc.c
set MKJAMBASE_SOURCES=mkjambase.c
set BJAM_SOURCES=
set BJAM_SOURCES=%BJAM_SOURCES% command.c compile.c constants.c debug.c
set BJAM_SOURCES=%BJAM_SOURCES% execcmd.c execnt.c filent.c frames.c function.c
set BJAM_SOURCES=%BJAM_SOURCES% glob.c hash.c hdrmacro.c headers.c jam.c
set BJAM_SOURCES=%BJAM_SOURCES% jambase.c jamgram.c lists.c make.c make1.c
set BJAM_SOURCES=%BJAM_SOURCES% object.c option.c output.c parse.c pathnt.c
set BJAM_SOURCES=%BJAM_SOURCES% pathsys.c regexp.c rules.c scan.c search.c
set BJAM_SOURCES=%BJAM_SOURCES% subst.c timestamp.c variable.c modules.c
set BJAM_SOURCES=%BJAM_SOURCES% strings.c filesys.c builtins.c md5.c class.c
set BJAM_SOURCES=%BJAM_SOURCES% cwd.c w32_getreg.c native.c modules/set.c
set BJAM_SOURCES=%BJAM_SOURCES% modules/path.c modules/regex.c
set BJAM_SOURCES=%BJAM_SOURCES% modules/property-set.c modules/sequence.c
set BJAM_SOURCES=%BJAM_SOURCES% modules/order.c

set BJAM_UPDATE=
:Check_Update
call :Test_Empty %1
if not errorlevel 1 goto Check_Update_End
call :Clear_Error
setlocal
set test=%1
set test=###%test%###
set test=%test:"###=%
set test=%test:###"=%
set test=%test:###=%
if "%test%" == "--update" goto Found_Update
endlocal
shift
if not "_%BJAM_UPDATE%_" == "_update_" goto Check_Update
:Found_Update
endlocal
set BJAM_UPDATE=update
:Check_Update_End
if "_%BJAM_UPDATE%_" == "_update_" (
    if not exist ".\bootstrap\jam0.exe" (
        set BJAM_UPDATE=
    )
)

@echo ON
@if "_%BJAM_UPDATE%_" == "_update_" goto Skip_Bootstrap
if exist bootstrap rd /S /Q bootstrap
md bootstrap
@if not exist jamgram.y goto Bootstrap_GrammarPrep
@if not exist jamgramtab.h goto Bootstrap_GrammarPrep
@goto Skip_GrammarPrep
:Bootstrap_GrammarPrep
%BOOST_JAM_CC% %BOOST_JAM_OPT_YYACC% %YYACC_SOURCES%
@if not exist ".\bootstrap\yyacc0.exe" goto Skip_GrammarPrep
.\bootstrap\yyacc0 jamgram.y jamgramtab.h jamgram.yy
:Skip_GrammarPrep
@if not exist jamgram.c goto Bootstrap_GrammarBuild
@if not exist jamgram.h goto Bootstrap_GrammarBuild
@goto Skip_GrammarBuild
:Bootstrap_GrammarBuild
@echo OFF
if "_%YACC%_" == "__" (
    call :Guess_Yacc
)
if errorlevel 1 goto Finish
@echo ON
%YACC% jamgram.y
@if errorlevel 1 goto Finish
del /f jamgram.c
rename y.tab.c jamgram.c
del /f jamgram.h
rename y.tab.h jamgram.h
:Skip_GrammarBuild
@echo ON
@if exist jambase.c goto Skip_Jambase
%BOOST_JAM_CC% %BOOST_JAM_OPT_MKJAMBASE% %MKJAMBASE_SOURCES%
@if not exist ".\bootstrap\mkjambase0.exe" goto Skip_Jambase
.\bootstrap\mkjambase0 jambase.c Jambase
:Skip_Jambase
%BOOST_JAM_CC% %BOOST_JAM_OPT_JAM% %BJAM_SOURCES%
:Skip_Bootstrap
@if not exist ".\bootstrap\jam0.exe" goto Skip_Jam
@set args=%*
@echo OFF
:Set_Args
setlocal
call :Test_Empty %args%
if not errorlevel 1 goto Set_Args_End
set test=###%args:~0,2%###
set test=%test:"###=%
set test=%test:###"=%
set test=%test:###=%
set test=%test:~0,1%
if "-" == "%test%" goto Set_Args_End
endlocal
set args=%args:~1%
goto Set_Args
:Set_Args_End
@echo ON
@if "_%BJAM_UPDATE%_" == "_update_" goto Skip_Clean
.\bootstrap\jam0 -f build.jam --toolset=%BOOST_JAM_TOOLSET% "--toolset-root=%BOOST_JAM_TOOLSET_ROOT% " %args% clean
:Skip_Clean
.\bootstrap\jam0 -f build.jam --toolset=%BOOST_JAM_TOOLSET% "--toolset-root=%BOOST_JAM_TOOLSET_ROOT% " %args%
:Skip_Jam

:Finish
