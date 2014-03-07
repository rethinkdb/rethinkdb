rem  Build a branches/release snapshot for POSIX, using LF line termination

rem  Copyright 2008 Beman Dawes

rem  Distributed under the Boost Software License, Version 1.0.
rem  See http://www.boost.org/LICENSE_1_0.txt

echo Build a branches/release snapshot for POSIX, using LF line termination...
echo Revision %BOOST_REVISION_NUMBER%
echo Removing old files...
rmdir /s /q posix >nul
rmdir /s /q svn_info >nul
del posix.tar.gz >nul
del posix.tar.bz2 >nul

echo Exporting files from subversion...
svn export --non-interactive --native-eol LF -r %BOOST_REVISION_NUMBER% http://svn.boost.org/svn/boost/branches/release posix

echo Copying docs into posix...
pushd posix
xcopy /s /y ..\docs_temp
popd

echo Setting SNAPSHOT_DATE
strftime "%%Y-%%m-%%d" >date.txt
set /p SNAPSHOT_DATE= <date.txt
echo SNAPSHOT_DATE is %SNAPSHOT_DATE%

echo Renaming root directory...
ren posix boost-posix-%SNAPSHOT_DATE%

echo Building .gz file...
tar cfz posix.tar.gz --numeric-owner --group=0 --owner=0 boost-posix-%SNAPSHOT_DATE%
echo Building .bz2 file...
gzip -d -c posix.tar.gz | bzip2 >posix.tar.bz2

ren boost-posix-%SNAPSHOT_DATE% posix

echo The ftp transfer will be done in two steps because that has proved more
echo reliable on Beman's Windows XP 64-bit system.

echo Creating ftp script 1 ...
copy user.txt posix.ftp
echo dir >>posix.ftp
echo binary >>posix.ftp

rem echo put posix.tar.gz >>posix.ftp
rem echo mdelete boost-posix*.gz >>posix.ftp
rem echo rename posix.tar.gz boost-posix-%SNAPSHOT_DATE%.tar.gz >>posix.ftp

echo put posix.tar.bz2 >>posix.ftp
echo bye >>posix.ftp

echo Running ftp script 1 ...
ftp -n -i -s:posix.ftp boost.cowic.de

echo Creating ftp script 2 ...
copy user.txt posix.ftp
echo dir >>posix.ftp
echo mdelete boost-posix*.bz2 >>posix.ftp
echo rename posix.tar.bz2 boost-posix-%SNAPSHOT_DATE%.tar.bz2 >>posix.ftp

echo dir >>posix.ftp
echo bye >>posix.ftp

echo Running ftp script 2 ...
ftp -n -i -s:posix.ftp boost.cowic.de

echo POSIX snapshot complete!
