#!/usr/bin/perl
#  ********************************************************************
#  * COPYRIGHT:
#  * Copyright (c) 2002-2008, International Business Machines
#  * Corporation and others. All Rights Reserved.
#  ********************************************************************

#use strict;

require "../perldriver/Common.pl";

use lib '../perldriver';

use PerfFramework;

# This test only works on Windows.
if (!$OnWindows) {
	print "This test only works on Windows.\n";
	exit(1);
}

my $options = {
	       "title"=>"Conversion performance regression: ICU (".$ICUPreviousVersion." and ".$ICULatestVersion.")",
           "headers"=>"ICU".$ICUPreviousVersion." ICU".$ICULatestVersion,
	       "operationIs"=>"code point",
	       "timePerOperationIs"=>"Time per code point",
	       "passes"=>"10",
	       "time"=>"5",
	       #"outputType"=>"HTML",
	       "dataDir"=>"Not Using Data Files",
	       "outputDir"=>"../results"
	      };

# programs

my $p1 = $ICUPathPrevious."/convperf/$WindowsPlatform/Release/convperf.exe"; # Previous
my $p2 = $ICUPathLatest."/convperf/$WindowsPlatform/Release/convperf.exe"; # Latest

my $dataFiles = "";


my $tests = { 
	     "UTF-8 From Unicode",          ["$p1 TestICU_UTF8_FromUnicode"  ,    "$p2 TestICU_UTF8_FromUnicode" ],
	     "UTF-8 To Unicode",            ["$p1 TestICU_UTF8_ToUnicode"    ,    "$p2 TestICU_UTF8_ToUnicode" ],
         ####
	     "ISO-8859-1 From Unicode",     ["$p1 TestICU_Latin1_FromUnicode"  ,  "$p2 TestICU_Latin1_FromUnicode" ],
	     "ISO-8859-1 To Unicode",       ["$p1 TestICU_Latin1_ToUnicode"    ,  "$p2 TestICU_Latin1_ToUnicode" ],
         ####
	     "Shift-JIS From Unicode",      ["$p1 TestICU_SJIS_FromUnicode"  ,    "$p2 TestICU_SJIS_FromUnicode" ],
	     "Shift-JIS To Unicode",        ["$p1 TestICU_SJIS_ToUnicode"    ,    "$p2 TestICU_SJIS_ToUnicode" ],
         ####
	     "EUC-JP From Unicode",         ["$p1 TestICU_EUCJP_FromUnicode"  ,   "$p2 TestICU_EUCJP_FromUnicode" ],
	     "EUC-JP To Unicode",           ["$p1 TestICU_EUCJP_ToUnicode"    ,   "$p2 TestICU_EUCJP_ToUnicode" ],
         ####
	     "GB2312 From Unicode",         ["$p1 TestICU_GB2312_FromUnicode"  ,  "$p2 TestICU_GB2312_FromUnicode" ],
	     "GB2312 To Unicode",           ["$p1 TestICU_GB2312_ToUnicode"    ,  "$p2 TestICU_GB2312_ToUnicode" ],
         ####
	     "ISO2022KR From Unicode",      ["$p1 TestICU_ISO2022KR_FromUnicode",  "$p2 TestICU_ISO2022KR_FromUnicode" ],
	     "ISO2022KR To Unicode",        ["$p1 TestICU_ISO2022KR_ToUnicode"  ,  "$p2 TestICU_ISO2022KR_ToUnicode" ],
         ####
	     "ISO2022JP From Unicode",      ["$p1 TestICU_ISO2022JP_FromUnicode", "$p2 TestICU_ISO2022JP_FromUnicode" ],
	     "ISO2022JP To Unicode",        ["$p1 TestICU_ISO2022JP_ToUnicode"  ,  "$p2 TestICU_ISO2022JP_ToUnicode" ],
	    };
	    

runTests($options, $tests, $dataFiles);


