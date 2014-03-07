rem Inspect Trunk
rem Copyright Beman Dawes 2008, 2009

rem Distributed under the Boost Software License, Version 1.0.
rem See http://www.boost.org/LICENSE_1_0.txt

echo Must be run in directory containing svn checkout of trunk

echo Clean trunk working copy ...
rem cleanup clears locks or other residual problems (we learned this the hard way!)
svn cleanup
echo Update trunk working copy...
svn up --non-interactive --trust-server-cert
pushd tools\inspect\build
echo Build inspect program...
bjam
popd
echo Copy inspect.exe to %UTIL% directory...
copy /y dist\bin\inspect.exe %UTIL%
echo Inspect...
inspect >%TEMP%\trunk_inspect.html

echo Create ftp script...
pushd %TEMP%
copy %BOOST_TRUNK%\..\user.txt inspect.ftp
echo dir >>inspect.ftp
echo binary >>inspect.ftp
echo put trunk_inspect.html >>inspect.ftp
echo dir >>inspect.ftp
echo mdelete inspect-trunk.html >>inspect.ftp
echo rename trunk_inspect.html inspect-trunk.html >>inspect.ftp
echo dir >>inspect.ftp
echo bye >>inspect.ftp

echo Run ftp script...
ftp -n -i -s:inspect.ftp boost.cowic.de
popd

echo Update script for next run
copy /y tools\release\inspect_trunk.bat

echo Inspect script complete
