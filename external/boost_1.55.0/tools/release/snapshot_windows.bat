rem  Build a branches/release snapshot for Windows, using CRLF line termination

rem  Copyright 2008 Beman Dawes

rem  Distributed under the Boost Software License, Version 1.0.
rem  See http://www.boost.org/LICENSE_1_0.txt

echo Build a branches/release snapshot for Windows, using CRLF line termination...
echo Revision %BOOST_REVISION_NUMBER%

echo Removing old files...
rmdir /s /q windows >nul
rmdir /s /q svn_info >nul
del windows.7z >nul
del windows.zip >nul

echo Exporting files from subversion...
svn export --non-interactive --native-eol CRLF -r %BOOST_REVISION_NUMBER% http://svn.boost.org/svn/boost/branches/release windows

echo Copying docs into windows...
pushd windows
xcopy /s /y ..\docs_temp
popd

echo Setting SNAPSHOT_DATE
strftime "%%Y-%%m-%%d" >date.txt
set /p SNAPSHOT_DATE= <date.txt
echo SNAPSHOT_DATE is %SNAPSHOT_DATE%

echo Renaming root directory...
ren windows boost-windows-%SNAPSHOT_DATE%

echo Building .7z file...
rem On Windows, 7z comes from the 7-Zip package, not Cygwin,
rem so path must include C:\Program Files\7-Zip
7z a -r windows.7z boost-windows-%SNAPSHOT_DATE%

rem Building .zip file...
rem zip -r windows.zip boost-windows-%SNAPSHOT_DATE%

ren boost-windows-%SNAPSHOT_DATE% windows

echo The ftp transfer will be done in two steps because that has proved more
echo reliable on Beman's Windows XP 64-bit system.

echo Creating ftp script 1 ...
rem user.txt must be a single line: user userid password
rem where "userid" and "password" are replace with the appropriate values
copy user.txt windows.ftp
echo dir >>windows.ftp
echo binary >>windows.ftp

rem echo put windows.zip >>windows.ftp
rem echo mdelete boost-windows*.zip >>windows.ftp
rem echo rename windows.zip boost-windows-%SNAPSHOT_DATE%.zip >>windows.ftp

echo put windows.7z >>windows.ftp
echo bye >>windows.ftp

echo Running ftp script 1 ...
ftp -n -i -s:windows.ftp boost.cowic.de

echo Creating ftp script 2 ...
copy user.txt windows.ftp
echo dir >>windows.ftp
echo mdelete boost-windows*.7z >>windows.ftp
echo rename windows.7z boost-windows-%SNAPSHOT_DATE%.7z >>windows.ftp

echo dir >>windows.ftp
echo bye >>windows.ftp

echo Running ftp script 2 ...
ftp -n -i -s:windows.ftp boost.cowic.de

echo Windows snapshot complete!
