rem Copyright Beman Dawes 2005

rem Distributed under the Boost Software License, Version 1.0.
rem See http://www.boost.org/LICENSE_1_0.txt

set TEST_LOCATE_ROOT=%TEMP%

echo Begin test processing...
bjam --dump-tests "-sALL_LOCATE_TARGET=%TEST_LOCATE_ROOT%" %* >bjam.log 2>&1
echo Begin log processing...
process_jam_log %TEST_LOCATE_ROOT% <bjam.log
start bjam.log
echo Begin compiler status processing...
compiler_status --locate-root %TEST_LOCATE_ROOT% %BOOST_ROOT% test_status.html test_links.html
start test_status.html
