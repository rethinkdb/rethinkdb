@echo off
rem  Build release packages

rem  Copyright Beman Dawes 2009

rem  Distributed under the Boost Software License, Version 1.0.
rem  See http://www.boost.org/LICENSE_1_0.txt

echo Build release packages...

if not %1$==$ goto ok
echo Usage: build_release_packages release-name
echo Example: build_release_packages boost_1_38_0_Beta1
goto done

:ok

echo Preping posix...
rmdir /s /q posix\bin.v2 2>nul
rmdir /s /q posix\dist 2>nul
ren posix %1
del %1.tar.gz 2>nul
del %1.tar.bz2 2>nul
echo Creating gz...
tar cfz %1.tar.gz %1
echo Creating bz2...
gzip -d -c %1.tar.gz | bzip2 >%1.tar.bz2
echo Cleaning up posix...
ren %1 posix

echo Preping windows...
rmdir /s /q windows\bin.v2 2>nul
rmdir /s /q windows\dist 2>nul
ren windows %1
del %1.zip 2>nul
del %1.7z 2>nul
echo Creating zip...
zip -r -q %1.zip %1
echo Creating 7z...
7z a -r -bd %1.7z %1
echo Cleaning up windows...
ren %1 windows

grep "Revision:" snapshot.log
echo Build release packages complete

:done
