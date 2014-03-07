@echo off

rem Copyright Robert Ramey 2007

rem Distributed under the Boost Software License, Version 1.0.
rem See http://www.boost.org/LICENSE_1_0.txt

if not "%1" == "" goto bjam
    echo Usage: %0 "<bjam arguments>"
    echo where typical bjam arguements are:
    echo   toolset=msvc-7.1,gcc
    echo   variant=debug,release,profile
    echo   link=static,shared
    echo   threading=single,multi
    echo   -sBOOST_ARCHIVE_LIST="<archive name>"
    goto end
:bjam
    bjam --dump-tests %* >bjam.log 2>&1
    process_jam_log --v2 <bjam.log
    library_status library_status.html links.html
:end
