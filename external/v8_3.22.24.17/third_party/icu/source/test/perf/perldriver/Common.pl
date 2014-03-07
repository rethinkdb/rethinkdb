#!/usr/bin/perl
#  ********************************************************************
#  * COPYRIGHT:
#  * Copyright (c) 2010, International Business Machines Corporation and
#  * others. All Rights Reserved.
#  ********************************************************************

# Settings by user
$ICULatestVersion = "";     # Change to respective version number
$ICUPreviousVersion = "";   # Change to respective version number
$ICUPrevious2Version = "";  # Change to respective version number

$PerformanceDataPath = "";          #Change to Performance Data Path

$ICULatest = "";      # Change to path of latest ICU (e.g. /home/user/Desktop/icu-4.0)
$ICUPrevious = "";    # Change to path of previous ICU
$ICUPrevious2 = "";   # Change to path of ICU before previous release

$OnWindows = 0;           # Change to 1 if on Windows
$Windows64 = 0;           # Change to 1 if on Windows and running 64 bit
# End of settings by user

$CollationDataPath = $PerformanceDataPath."/collation";    # Collation Performance Data Path
$ConversionDataPath = $PerformanceDataPath."/conversion";  # Conversion Performance Data Path
$UDHRDataPath = $PerformanceDataPath."/udhr";              # UDHR Performance Data Path

$ICUPathLatest = $ICULatest."/source/test/perf";
$ICUPathPrevious = $ICUPrevious."/source/test/perf";
$ICUPathPrevious2 = $ICUPrevious."/source/test/perf";

$WindowsPlatform = "";
if ($Windows64) {
    $WindowsPlatform = "x64";
} else {
    $WindowsPlatform = "x86";
}

return 1;
