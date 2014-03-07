#!/usr/bin/perl
#  ********************************************************************
#  * COPYRIGHT:
#  * Copyright (c) 2002-2008, International Business Machines Corporation and
#  * others. All Rights Reserved.
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
	       "title"=>"Conversion Performance: ICU ".$ICULatestVersion." vs. Windows XP ANSI Interface",
	       "headers"=>"WindowsXP(IMultiLanguage2) ICU".$ICULatestVersion,
	       "operationIs"=>"code point",
	       "passes"=>"10",
	       "time"=>"5",
	       #"outputType"=>"HTML",
	       "dataDir"=>"Not Using Data Files",
	       "outputDir"=>"../results"
	      };

# programs
# tests will be done for all the programs. Results will be stored and connected
my $p = $ICUPathLatest."/convperf/$WindowsPlatform/Release/convperf.exe";

my $tests = { 
	     "UTF-8 From Unicode",          ["$p TestWinANSI_UTF8_FromUnicode"  ,    "$p TestICU_UTF8_FromUnicode" ],
	     "UTF-8 To Unicode",            ["$p TestWinANSI_UTF8_ToUnicode"    ,    "$p TestICU_UTF8_ToUnicode" ],
         ####
	     "ISO-8859-1 From Unicode",     ["$p TestWinANSI_Latin1_FromUnicode"  ,  "$p TestICU_Latin1_FromUnicode" ],
	     "ISO-8859-1 To Unicode",       ["$p TestWinANSI_Latin1_ToUnicode"    ,  "$p TestICU_Latin1_ToUnicode" ],
         ####
	     "Shift-JIS From Unicode",      ["$p TestWinANSI_SJIS_FromUnicode"  ,    "$p TestICU_SJIS_FromUnicode" ],
	     "Shift-JIS To Unicode",        ["$p TestWinANSI_SJIS_ToUnicode"    ,    "$p TestICU_SJIS_ToUnicode" ],
         ####
	     "EUC-JP From Unicode",         ["$p TestWinANSI_EUCJP_FromUnicode"  ,   "$p TestICU_EUCJP_FromUnicode" ],
	     "EUC-JP To Unicode",           ["$p TestWinANSI_EUCJP_ToUnicode"    ,   "$p TestICU_EUCJP_ToUnicode" ],
         ####
	     "GB2312 From Unicode",         ["$p TestWinANSI_GB2312_FromUnicode"  ,  "$p TestICU_GB2312_FromUnicode" ],
	     "GB2312 To Unicode",           ["$p TestWinANSI_GB2312_ToUnicode"    ,  "$p TestICU_GB2312_ToUnicode" ],
	    };

my $dataFiles = "";

runTests($options, $tests, $dataFiles);
