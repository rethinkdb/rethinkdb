#!/usr/bin/perl
#  ********************************************************************
#  * COPYRIGHT:
#  * Copyright (c) 2002-2008, International Business Machines
#  * Corporation and others. All Rights Reserved.
#  ********************************************************************

#use strict;

use lib '../perldriver';

require "../perldriver/Common.pl";

use PerfFramework;

my $options = {
	       "title"=>"Unicode String performance regression: ICU (".$ICUPreviousVersion." and ".$ICULatestVersion.")",
           "headers"=>"ICU".$ICUPreviousVersion." ICU".$ICULatestVersion,
	       "operationIs"=>"Unicode String",
	       "passes"=>"10",
	       "time"=>"5",
	       #"outputType"=>"HTML",
	       "dataDir"=>$CollationDataPath,
           "outputDir"=>"../results"
	      };

# programs

my $p1; # Previous
my $p2; # Latest

if ($OnWindows) {
	$p1 = $ICUPathPrevious."/ustrperf/$WindowsPlatform/Release/stringperf.exe -b -u"; # Previous
    $p2 = $ICUPathLatest."/ustrperf/$WindowsPlatform/Release/stringperf.exe -b -u"; # Latest
} else {
	$p1 = $ICUPathPrevious."/ustrperf/stringperf -b -u"; # Previous
    $p2 = $ICUPathLatest."/ustrperf/stringperf -b -u"; # Latest
}

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
		  "TestNames_SerbianSH.txt",
		  "TestNames_SerbianSR.txt",
		  "TestNames_Thai.txt",
		  "Testnames_Russian.txt",
		  "th18057.txt",
		  "thesis.txt",
		  "vfear11a.txt",
		 ]
		};


my $tests = { 
"Object Construction(empty string)",      ["$p1 TestCtor"         , "$p2 TestCtor"         ],
"Object Construction(single char)",       ["$p1 TestCtor1"        , "$p2 TestCtor1"        ],
"Object Construction(another string)",    ["$p1 TestCtor2"        , "$p2 TestCtor2"        ],
"Object Construction(string literal)",    ["$p1 TestCtor3"        , "$p2 TestCtor3"        ],
"String Assignment(helper)",     		  ["$p1 TestAssign"       , "$p2 TestAssign"       ],
"String Assignment(string literal)",      ["$p1 TestAssign1"      , "$p2 TestAssign1"      ],
"String Assignment(another string)",      ["$p1 TestAssign2"      , "$p2 TestAssign2"      ],
"Get String or Character",      		  ["$p1 TestGetch"        , "$p2 TestGetch"        ],
"Concatenation",   						  ["$p1 TestCatenate"     , "$p2 TestCatenate"     ],
"String Scanning(char)",     		      ["$p1 TestScan"         , "$p2 TestScan"         ],
"String Scanning(string)",       		  ["$p1 TestScan1"        , "$p2 TestScan1"        ],
"String Scanning(char set)",       	      ["$p1 TestScan2"        , "$p2 TestScan2"        ],
};


runTests($options, $tests, $dataFiles);


