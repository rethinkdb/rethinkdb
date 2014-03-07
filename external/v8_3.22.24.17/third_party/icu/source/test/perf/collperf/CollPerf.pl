#!/usr/bin/perl
#  ********************************************************************
#  * COPYRIGHT:
#  * Copyright (c) 2005-2008, International Business Machines Corporation and
#  * others. All Rights Reserved.
#  ********************************************************************

#use strict;

use lib '../perldriver';

require "../perldriver/Common.pl";

use PerfFramework;

# This test should be run on Windows.
if (!$OnWindows) {
	print "This test should be run on Windows.\n";
    exit(1);
}

my $options = {
	       "title"=>"Collation performance: ICU,POSIX,and Win",
	       "headers"=>"ICU POSIX WIN",
	       "operationIs"=>"unicode String",
	       "passes"=>"1",
	       "time"=>"2",
	       #"outputType"=>"HTML",
	       "dataDir"=>$CollationDataPath,
           "outputDir"=>"../results"
	      };

# programs
# tests will be done for all the programs. Results will be stored and connected
my $p = $ICUPathLatest."/collperf/$WindowsPlatform/Release/collperf.exe";

my $tests = { 
	     "Key Gen null",  ["$p TestIcu_KeyGen_null", "$p TestPosix_KeyGen_null", "$p TestWin_KeyGen_null"],
	     "qsort strcoll null",  ["$p TestIcu_qsort_strcoll_null", "$p TestPosix_qsort_strcoll_null", "$p TestWin_qsort_CompareStringW_null"],
	     "qsort use key",  ["$p TestIcu_qsort_usekey", "$p TestPosix_qsort_usekey", "$p TestWin_qsort_usekey"],
	     "Binary Search icu strcoll null",  ["$p TestIcu_BinarySearch_strcoll_null", "$p TestPosix_BinarySearch_strcoll_null", "$p TestWin_BinarySearch_CompareStringW_null"],
	     "Binary Search icu use key",  ["$p TestIcu_BinarySearch_usekey", "$p TestPosix_BinarySearch_usekey", "$p TestWin_BinarySearch_usekey"],
	     # These are the original test. They are commented out to so that the above test can run and compare certain aspects of collation.
	     #"Key Gen ICU null",  ["$p TestIcu_KeyGen_null"],
	     #"Key Gen ICU len",  ["$p TestIcu_KeyGen_len"],
	     #"Key Gen POSIX",  ["$p TestPosix_KeyGen_null"],
	     #"Key Gen Win",  ["$p TestWin_KeyGen_null"],
	     #"Iteration icu forward null",  ["$p TestIcu_ForwardIter_null"],
	     #"Iteration icu forward len",  ["$p TestIcu_ForwardIter_len"],
	     #"Iteration icu backward null",  ["$p TestIcu_BackwardIter_null"],
	     #"Iteration icu backward len",  ["$p TestIcu_BackwardIter_len"],
	     #"Iteration/all icu forward null",  ["$p TestIcu_ForwardIter_all_null"],
	     #"Iteration/all icu forward len",  ["$p TestIcu_ForwardIter_all_len"],
	     #"Iteration/all icu backward null",  ["$p TestIcu_BackwardIter_all_null"],
	     #"Iteration/all icu backward len",  ["$p TestIcu_BackwardIter_all_len"],
	     #"qsort icu strcoll null",  ["$p TestIcu_qsort_strcoll_null"],
	     #"qsort icu strcoll len",  ["$p TestIcu_qsort_strcoll_len"],
	     #"qsort icu use key",  ["$p TestIcu_qsort_usekey"],
	     #"qsort posix strcoll null",  ["$p TestPosix_qsort_strcoll_null"],
	     #"qsort posix use key",  ["$p TestPosix_qsort_usekey"],
	     #"qsort win CompareStringW null",  ["$p TestWin_qsort_CompareStringW_null"],
	     #"qsort win CompareStringW len",  ["$p TestWin_qsort_CompareStringW_len"],
	     #"qsort win use key",  ["$p TestWin_qsort_usekey"],
	     #"Binary Search icu strcoll null",  ["$p TestIcu_BinarySearch_strcoll_null"],
	     #"Binary Search icu strcoll len",  ["$p TestIcu_BinarySearch_strcoll_len"],
	     #"Binary Search icu use key",  ["$p TestIcu_BinarySearch_usekey"],
	     #"Binary Search icu u_strcmp",  ["$p TestIcu_BinarySearch_strcmp"],
	     #"Binary Search icu cmpCPO",  ["$p TestIcu_BinarySearch_cmpCPO"],
	     #"Binary Search posix strcoll null",  ["$p TestPosix_BinarySearch_strcoll_null"],
	     #"Binary Search posix use key",  ["$p TestPosix_BinarySearch_usekey"],
	     #"Binary Search win CompareStringW null",  ["$p TestWin_BinarySearch_CompareStringW_null"],
	     #"Binary Search win CompareStringW len",  ["$p TestWin_BinarySearch_CompareStringW_len"],
	     #"Binary Search win use key",  ["$p TestWin_BinarySearch_usekey"],
	     #"Binary Search win wcscmp",  ["$p TestWin_BinarySearch_wcscmp"],
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
