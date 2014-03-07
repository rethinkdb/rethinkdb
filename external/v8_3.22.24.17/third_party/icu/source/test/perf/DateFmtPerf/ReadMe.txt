**********************************************************************
* Copyright (c) 2002-2010,International Business Machines
* Corporation and others.  All Rights Reserved.
**********************************************************************
**********************************************************************


The purpose of this performance test is to test the "real world" applications of ICU, such as Date Formatting and the Break Iterator.  In both of these cases, the performance test function does all of the work, i.e. initializing, formatting, etc.

There is no Perl script associated with this performance test, due to the fact that the performance test results in a different time if it is allowed to run more than once per execution of the executable.  We are only interested in the first time returned by the executable in order to maintain accurate "real world" results.  For this to happen, make sure to run the executable with the -i 1 and -p 1 options.

There are 7 tests contained in this performance test:
DateFmt250: Tests date formatting with 250 dates
DateFmt10000: Tests date formatting with 10,000 dates
DateFmt100000: Tests date formatting with 100,000 dates
BreakItWord250: Tests word break iteration with 250 iterations.
BreakItWord10000: Tests word break iteration with 10000 iterations.
BreakItChar250: Tests character break iteration with 250 iterations.
BreakItChar10000: Tests character break iteration with 10000 iterations.

For example:
datefmtperf.exe -i 1 -p 1 DateFmt250