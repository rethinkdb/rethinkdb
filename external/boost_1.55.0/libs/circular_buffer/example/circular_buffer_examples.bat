echo off
rem quickbook doxgen auto-index docs template circular_buffer_html_index.bat
rem echo circular_buffer_html_index_%date%_%time:~0,2%_%time:~3,2%.log
rem The DOS time format is assumed 12:34 and the : separator is not used.
set t=%time% /T
set tim=%t:~0,2%%t:~3,2%
rem pick just hours and minutes.
rem time may include leading space, like " 915", so remove space.
set tim=%tim: =%
rem boost-no-inspect
rem cd \boost-trunk/circular_buffer\libs\circular_buffer\example
bjam -a > circular_buffer_examples_%date%_%tim%.log
if not ERRORLEVEL 0 (echo Errorlevel is %ERRORLEVEL%) else (echo OK)
pause
