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
	       "title"=>"UnicodeSet span()/contains() performance",
	       "headers"=>"Bv Bv0",
	       "operationIs"=>"tested Unicode code point",
	       "passes"=>"3",
	       "time"=>"2",
	       #"outputType"=>"HTML",
	       "dataDir"=>$UDHRDataPath,
           "outputDir"=>"../results"
	      };

# programs
# tests will be done for all the programs. Results will be stored and connected
my $p;
if ($OnWindows) {
	$p = $ICUPathLatest."/unisetperf/$WindowsPlatform/Release/unisetperf.exe";
} else {
	$p = $ICUPathLatest."/unisetperf/unisetperf";
}
my $pc =  "$p Contains";
my $p16 = "$p SpanUTF16";
my $p8 =  "$p SpanUTF8";

my $tests = {
	     "Contains",  ["$pc  --type Bv",
	                   "$pc  --type Bv0"
	                   ],
	     "SpanUTF16", ["$p16 --type Bv",
	                   "$p16 --type Bv0"
	                   ]
	    };

my $dataFiles = {
		 "",
		 [
		  "udhr_eng.txt",
          "udhr_deu_1996.txt",
          "udhr_fra.txt",
          "udhr_rus.txt",
          "udhr_tha.txt",
          "udhr_jpn.txt",
          "udhr_cmn_hans.txt",
          "udhr_cmn_hant.txt",
          "udhr_jpn.html"
		 ]
		};

runTests($options, $tests, $dataFiles);

$options = {
	       "title"=>"UnicodeSet span()/contains() performance",
	       "headers"=>"Bv BvF Bvp BvpF L Bvl",
	       "operationIs"=>"tested Unicode code point",
	       "passes"=>"3",
	       "time"=>"2",
	       #"outputType"=>"HTML",
	       "dataDir"=>$UDHRDataPath,
	       "outputDir"=>"../results"
	      };

$tests = {
	     "SpanUTF8",  ["$p8  --type Bv",
	                   "$p8  --type BvF",
	                   "$p8  --type Bvp",
	                   "$p8  --type BvpF",
	                   "$p8  --type L",
	                   "$p8  --type Bvl"
	                   ]
	    };

runTests($options, $tests, $dataFiles);
