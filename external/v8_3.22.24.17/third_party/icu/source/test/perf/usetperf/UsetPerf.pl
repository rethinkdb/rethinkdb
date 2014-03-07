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
	       "title"=>"Uset performance: ICU (".$ICUPreviousVersion." and ".$ICULatestVersion.")",
           "headers"=>"ICU".$ICUPreviousVersion." ICU".$ICULatestVersion,
	       "operationIs"=>"unicode string",
	       "passes"=>"1",
	       "time"=>"2",
	       #"outputType"=>"HTML",
	       "dataDir"=>"Not Using Data Files",
           "outputDir"=>"../results"
	      };

# programs
# tests will be done for all the programs. Results will be stored and connected
my $p1, $p2;
if ($OnWindows) {
	$p1 = $ICUPathPrevious."/usetperf/$WindowsPlatform/Release/usetperf.exe";
	$p2 = $ICUPathLatest."/usetperf/$WindowsPlatform/Release/usetperf.exe";
} else {
	$p1 = $ICUPathPrevious."/usetperf/usetperf";
	$p2 = $ICUPathLatest."/usetperf/usetperf";
	
} 

my $tests = { 
	     "titlecase_letter/add",  ["$p1 titlecase_letter_add", "$p2 titlecase_letter_add"],
	     "titlecase_letter/contains",  ["$p1 titlecase_letter_contains", "$p2 titlecase_letter_contains"],
	     "titlecase_letter/iterator",  ["$p1 titlecase_letter_iterator", "$p2 titlecase_letter_iterator"],
	     "unassigned/add",  ["$p1 unassigned_add", "$p2 unassigned_add"],
	     "unassigned/contains",  ["$p1 unassigned_contains", "$p2 unassigned_contains"],
	     "unassigned/iterator",  ["$p1 unassigned_iterator", "$p2 unassigned_iterator"],
	     "pattern1",  ["$p1 pattern1", "$p2 pattern1"],
	     "pattern2",  ["$p1 pattern2", "$p2 pattern2"],
	     "pattern3",  ["$p1 pattern3", "$p2 pattern3"],
	    };

my $dataFiles = {
		};

runTests($options, $tests, $dataFiles);
