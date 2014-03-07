#!/usr/bin/perl
#  ********************************************************************
#  * COPYRIGHT:
#  * Copyright (c) 2008, International Business Machines Corporation and
#  * others. All Rights Reserved.
#  ********************************************************************

#use strict;

use lib '../perldriver';

require "../perldriver/Common.pl";

use PerfFramework;

my $options = {
	       "title"=>"Collation performanceregression: ICU (".$ICUPreviousVersion." and ".$ICULatestVersion.")",
           "headers"=>"ICU".$ICUPreviousVersion." ICU".$ICULatestVersion,
	       "operationIs"=>"unicode String",
	       "passes"=>"1",
	       "time"=>"2",
	       #"outputType"=>"HTML",
	       "dataDir"=>$CollationDataPath,
           "outputDir"=>"../results"
	      };

# programs
# tests will be done for all the programs. Results will be stored and connected
my $p1, $p2;

if ($OnWindows) {
	$p1 = $ICUPathPrevious."/collperf/$WindowsPlatform/Release/collperf.exe";
	$p2 = $ICUPathLatest."/collperf/$WindowsPlatform/Release/collperf.exe";
} else {
	$p1 = $ICUPathPrevious."/collperf/collperf";
	$p2 = $ICUPathLatest."/collperf/collperf";
}


my $tests = { 
	     "Key Gen ICU null",  ["$p1 TestIcu_KeyGen_null", "$p2 TestIcu_KeyGen_null"],
	     "Key Gen ICU len",  ["$p1 TestIcu_KeyGen_len", "$p2 TestIcu_KeyGen_len"],
	     "Iteration icu forward null",  ["$p1 TestIcu_ForwardIter_null", "$p2 TestIcu_ForwardIter_null"],
	     "Iteration icu forward len",  ["$p1 TestIcu_ForwardIter_len", "$p2 TestIcu_ForwardIter_len"],
	     "Iteration icu backward null",  ["$p1 TestIcu_BackwardIter_null", "$p2 TestIcu_BackwardIter_null"],
	     "Iteration icu backward len",  ["$p1 TestIcu_BackwardIter_len", "$p2 TestIcu_BackwardIter_len"],
	     "Iteration/all icu forward null",  ["$p1 TestIcu_ForwardIter_all_null", "$p2 TestIcu_ForwardIter_all_null"],
	     "Iteration/all icu forward len",  ["$p1 TestIcu_ForwardIter_all_len", "$p2 TestIcu_ForwardIter_all_len"],
	     "Iteration/all icu backward null",  ["$p1 TestIcu_BackwardIter_all_null", "$p2 TestIcu_BackwardIter_all_null"],
	     "Iteration/all icu backward len",  ["$p1 TestIcu_BackwardIter_all_len", "$p2 TestIcu_BackwardIter_all_len"],
	     "qsort icu strcoll null",  ["$p1 TestIcu_qsort_strcoll_null", "$p2 TestIcu_qsort_strcoll_null"],
	     "qsort icu strcoll len",  ["$p1 TestIcu_qsort_strcoll_len", "$p2 TestIcu_qsort_strcoll_len"],
	     "qsort icu use key",  ["$p1 TestIcu_qsort_usekey", "$p2 TestIcu_qsort_usekey"],
	     "Binary Search icu strcoll null",  ["$p1 TestIcu_BinarySearch_strcoll_null", "$p2 TestIcu_BinarySearch_strcoll_null"],
	     "Binary Search icu strcoll len",  ["$p1 TestIcu_BinarySearch_strcoll_len", "$p2 TestIcu_BinarySearch_strcoll_len"],
	     "Binary Search icu use key",  ["$p1 TestIcu_BinarySearch_usekey", "$p2 TestIcu_BinarySearch_usekey"],
	     "Binary Search icu u_strcmp",  ["$p1 TestIcu_BinarySearch_strcmp", "$p2 TestIcu_BinarySearch_strcmp"],
	     "Binary Search icu cmpCPO",  ["$p1 TestIcu_BinarySearch_cmpCPO", "$p2 TestIcu_BinarySearch_cmpCPO"],
	    };

my $dataFiles = {
		 "",
		 [
		  "TestNames_Asian.txt",
		  "TestNames_Chinese.txt",
		  "TestNames_Japanese.txt",
		  "TestNames_Japanese_h.txt",
		  "TestNames_Japanese_k.txt",
		  "TestNames_Korean.txt",
		  "TestNames_Latin.txt",
		  "Testnames_Russian.txt",
		  "TestNames_SerbianSH.txt",
		  "TestNames_SerbianSR.txt",
		  "TestNames_Simplified_Chinese.txt",
		  "TestNames_Thai.txt"
		 ]
		};

runTests($options, $tests, $dataFiles);
