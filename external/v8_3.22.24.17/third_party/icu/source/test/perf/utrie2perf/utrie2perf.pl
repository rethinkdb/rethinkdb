#!/usr/bin/perl
#  ********************************************************************
#  * COPYRIGHT:
#  * Copyright (c) 2005-2008, International Business Machines Corporation and
#  * others. All Rights Reserved.
#  ********************************************************************

#use strict;

require "../perldriver/Common.pl";

use lib '../perldriver';

use PerfFramework;

my $options = {
	       "title"=>"UTF performance: ICU (".$ICUPreviousVersion." and ".$ICULatestVersion.")",
	       "headers"=>"ICU".$ICUPreviousVersion." ICU".$ICULatestVersion,
	       "operationIs"=>"gb18030 encoding string",
	       "passes"=>"1",
	       "time"=>"2",
	       #"outputType"=>"HTML",
	       "dataDir"=>$ConversionDataPath,
           "outputDir"=>"../results"
	      };

# programs
# tests will be done for all the programs. Results will be stored and connected
my $p1;
my $p2;

if ($OnWindows) {
    $p1 = $ICUPathPrevious."/utfperf/$WindowsPlatform/Release/utfperf.exe -e gb18030"; # Previous
    $p2 = $ICUPathLatest."/utfperf/$WindowsPlatform/Release/utfperf.exe -e gb18030"; # Latest
} else {
    $p1 = $ICUPathPrevious."/utfperf/utfperf -e gb18030"; # Previous
    $p2 = $ICUPathLatest."/utfperf/utfperf -e gb18030"; # Latest
}

my $tests = { 
	     "Roundtrip",      ["$p1 Roundtrip",        "$p2 Roundtrip"],
	     "FromUnicode",    ["$p1 FromUnicode",      "$p2 FromUnicode"],
	     "FromUTF8",       ["$p1 FromUTF8",         "$p2 FromUTF8"],
	     #"UTF-8",  ["$p UTF_8"],
	     #"UTF-8 small buffer",  ["$p UTF_8_SB"],
	     #"SCSU",  ["$p SCSU"],
	     #"SCSU small buffer",  ["$p SCSU_SB"],
	     #"BOCU_1",  ["$p BOCU_1"],
	     #"BOCU_1 small buffer",  ["$p BOCU_1_SB"],
	    };

my $dataFiles = {
		 "",
		 [
		  "xuzhimo.txt"
		 ]
		};

runTests($options, $tests, $dataFiles);
