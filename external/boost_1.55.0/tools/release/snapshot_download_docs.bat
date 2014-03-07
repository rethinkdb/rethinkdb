@echo off
rem  Download and unpack boost-release-docs.7z

rem  Copyright 2008 Beman Dawes

rem  Distributed under the Boost Software License, Version 1.0.
rem  See http://www.boost.org/LICENSE_1_0.txt

echo Downloading docs subdirectory...

echo Deleting old files and directories ...
del boost-docs.7z 2>nul
del boost-release-docs.7z 2>nul
rmdir /s /q docs_temp 2>nul 
mkdir docs_temp

echo Creating ftp script ...
rem user.txt must be a single line: user userid password
rem where "userid" and "password" are replace with the appropriate values
copy user.txt download_docs.ftp
echo binary >>download_docs.ftp
echo get boost-release-docs.7z >>download_docs.ftp
echo bye >>download_docs.ftp

echo Running ftp script ...
ftp -d -n -i -s:download_docs.ftp boost.cowic.de

echo Unpacking 7z file ...
7z x -y -odocs_temp boost-release-docs.7z

echo Download and unpack boost-release-docs.7z complete!