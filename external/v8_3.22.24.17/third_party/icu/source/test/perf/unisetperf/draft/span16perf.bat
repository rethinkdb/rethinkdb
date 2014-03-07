rem  Copyright (c) 2007, International Business Machines Corporation and
rem  others. All Rights Reserved.

set PERF=c:\svn\icuproj\icu\ucnvutf8\source\test\perf\unisetperf\release\unisetperf
rem types: slow Bv Bv0 B0
rem --pattern [:White_Space:]

for %%f in (udhr_eng.txt
            udhr_deu.txt
            udhr_fra.txt
            udhr_rus.txt
            udhr_tha.txt
            udhr_jpn.txt
            udhr_cmn.txt
            udhr_jpn.html) do (
  for %%t in (slow Bv Bv0) do (
    %PERF% SpanUTF16 --type %%t -f \temp\udhr\%%f --pattern [:White_Space:] -v -e UTF-8 --passes 3 --iterations 10000
  )
)
