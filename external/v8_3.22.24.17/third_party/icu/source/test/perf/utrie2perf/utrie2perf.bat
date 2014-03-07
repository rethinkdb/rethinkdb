rem Copyright (C) 2008, International Business Machines Corporation and others.
rem All Rights Reserved.

set PERF=c:\svn\icuproj\icu\utf8\source\test\perf\utrie2perf\x86\Release\utrie2perf

for %%f in (udhr_eng.txt
            udhr_deu.txt
            udhr_fra.txt
            udhr_rus.txt
            udhr_tha.txt
            udhr_jpn.txt
            udhr_cmn.txt
            udhr_jpn.html) do (
    %PERF% CheckFCD           -f \temp\udhr\%%f -v -e UTF-8 --passes 3 --iterations 30000
rem %PERF% CheckFCDAlwaysGet  -f \temp\udhr\%%f -v -e UTF-8 --passes 3 --iterations 30000
rem %PERF% CheckFCDUTF8       -f \temp\udhr\%%f -v -e UTF-8 --passes 3 --iterations 30000
    %PERF% ToNFC              -f \temp\udhr\%%f -v -e UTF-8 --passes 3 --iterations 30000
    %PERF% GetBiDiClass       -f \temp\udhr\%%f -v -e UTF-8 --passes 3 --iterations 30000
)
