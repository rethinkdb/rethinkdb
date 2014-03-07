#!/usr/bin/perl
#  ********************************************************************
#  * COPYRIGHT:
#  * Copyright (c) 2003-2008, International Business Machines Corporation and
#  * others. All Rights Reserved.
#  ********************************************************************


#use strict;

require "../perldriver/Common.pl";

use lib '../perldriver';

use PerfFramework;

my $options = {
	       "title"=>"Unicode String performance: ICU ".$ICULatestVersion." vs. STDLib",
	       "headers"=>"StdLib ICU".$ICULatestVersion,
	       "operationIs"=>"Unicode String",
	       "timePerOperationIs"=>"Time per Unicode String",
	       "passes"=>"5",
	       "time"=>"2",
	       #"outputType"=>"HTML",
	       "dataDir"=>$CollationDataPath,
           "outputDir"=>"../results"
	      };


# programs
# tests will be done for all the programs. Results will be stored and connected
my $p;
if ($OnWindows) {
	$p = $ICUPathLatest."/ustrperf/$WindowsPlatform/Release/stringperf.exe -l -u";
} else {
	$p = $ICUPathLatest."/ustrperf/stringperf -l -u";
}

my $tests = { 
"Object Construction(empty string)",      ["$p TestStdLibCtor"         , "$p TestCtor"         ],
"Object Construction(single char)",       ["$p TestStdLibCtor1"        , "$p TestCtor1"        ],
"Object Construction(another string)",    ["$p TestStdLibCtor2"        , "$p TestCtor2"        ],
"Object Construction(string literal)",    ["$p TestStdLibCtor3"        , "$p TestCtor3"        ],
"String Assignment(helper)",     		  ["$p TestStdLibAssign"       , "$p TestAssign"       ],
"String Assignment(string literal)",      ["$p TestStdLibAssign1"      , "$p TestAssign1"      ],
"String Assignment(another string)",      ["$p TestStdLibAssign2"      , "$p TestAssign2"      ],
"Get String or Character",      		  ["$p TestStdLibGetch"        , "$p TestGetch"        ],
"Concatenation",   						  ["$p TestStdLibCatenate"     , "$p TestCatenate"     ],
"String Scanning(char)",     		      ["$p TestStdLibScan"         , "$p TestScan"         ],
"String Scanning(string)",       		  ["$p TestStdLibScan1"        , "$p TestScan1"        ],
"String Scanning(char set)",       	      ["$p TestStdLibScan2"        , "$p TestScan2"        ],
};

my $dataFiles = {
		 "",
		 [
		  "TestNames_Asian.txt",
		  "TestNames_Chinese.txt",
		  "TestNames_Simplified_Chinese.txt",
		  "TestNames_Japanese_h.txt",
		  "TestNames_Japanese_k.txt",
		  "TestNames_Korean.txt",
		  "TestNames_Latin.txt",
		  "TestNames_SerbianSH.txt",
		  "TestNames_SerbianSR.txt",
		  "TestNames_Thai.txt",
		  "Testnames_Russian.txt",
		  "th18057.txt",
		 ]
		};

runTests($options, $tests, $dataFiles);

# The whole command line would be something like:
# 	stringperf.exe -p 5 -t 2 -f c:/src/data/perf/TestNames_Asian.txt -l -u TestStdLibCatenate