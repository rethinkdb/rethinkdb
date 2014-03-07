rem Inspect snapshot

rem Copyright Beman Dawes 2008, 2011

rem Distributed under the Boost Software License, Version 1.0.
rem See http://www.boost.org/LICENSE_1_0.txt

echo inspect...
pushd windows
rem inspect_trunk.bat builds inspect program every day and copies it to %UTIL%
%UTIL%%\inspect >..\inspect.html
popd

echo Create ftp script...
copy user.txt inspect.ftp
echo dir >>inspect.ftp
echo binary >>inspect.ftp
echo put inspect.html >>inspect.ftp
echo dir >>inspect.ftp
echo mdelete inspect-snapshot.html >>inspect.ftp
echo rename inspect.html inspect-snapshot.html >>inspect.ftp
echo dir >>inspect.ftp
echo bye >>inspect.ftp

echo Run ftp script...
ftp -n -i -s:inspect.ftp boost.cowic.de

echo Inspect script complete
