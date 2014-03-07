#!/usr/bin/perl
#  ********************************************************************
#  * COPYRIGHT:
#  * Copyright (c) 2002-2008, International Business Machines Corporation and
#  * others. All Rights Reserved.
#  ********************************************************************

require "../perldriver/Common.pl";

use lib '../perldriver';

my $p;
if ($OnWindows) {
    $p = $ICUPathLatest . "/collationperf/$WindowsPlatform/Release/collationperf.exe";
}
else {
    $p = $ICUPathLatest . "/collationperf/collperf";
}

my @locale = (
    "en_US",
    "da_DK",
    "de_DE",
    "fr_FR",
    "ja_JP",
    "ja_JP",
    "ja_JP",
    "ja_JP",
    "zh_CN",
    "zh_CN",
    "zh_CN",
    "zh_TW",
    "zh_TW",
    "ko_KR",
    "ko_KR",
    "ru_RU",
    "ru_RU",
    "th_TH",
    "th_TH"
);

my $filePath   = $CollationDataPath . "/";
my $filePrefix = "TestNames_";
my @data       = (
    $filePrefix."Latin.txt",
    $filePrefix."Latin.txt",
    $filePrefix."Latin.txt",
    $filePrefix."Latin.txt",
    $filePrefix."Latin.txt",
    $filePrefix."Japanese_h.txt",
    $filePrefix."Japanese_k.txt",
    $filePrefix."Asian.txt",
    $filePrefix."Latin.txt",
    $filePrefix."Chinese.txt",
    $filePrefix."Simplified_Chinese.txt",
    $filePrefix."Latin.txt",
    $filePrefix."Chinese.txt",
    $filePrefix."Latin.txt",
    $filePrefix."Korean.txt",
    $filePrefix."Latin.txt",
    $filePrefix."Russian.txt",
    $filePrefix."Latin.txt",
    $filePrefix."Thai.txt"
);

my @resultPER;
my @resultFIN;

for ( $n = 0 ; $n < @data ; $n++ ) {
    my $resultICU;
    my $resultNIX;
    $resultICU = @locale[$n].",".@data[$n].",";
    $resultNIX = @locale[$n].",".@data[$n].",";
    @resultFIN[$n] = @locale[$n].",".@data[$n].",";

    #quicksort
    my @icu = `$p -locale @locale[$n] -loop 1000 -file $filePath@data[$n] -qsort`;
    my @nix = `$p -locale @locale[$n] -unix -loop 1000 -file $filePath@data[$n] -qsort`;

    my @icua = split( ' = ', $icu[2] );
    my @icub = split( ' ',   $icua[1] );
    my @nixa = split( ' = ', $nix[2] );
    my @nixb = split( ' ',   $nixa[1] );

    $resultICU = $resultICU.$icub[0].",";
    $resultNIX = $resultNIX.$nixb[0].",";

    #keygen time
    @icu = `$p -locale @locale[$n] -loop 1000 -file $filePath@data[$n] -keygen`;
    @nix = `$p -locale @locale[$n] -unix -loop 1000 -file $filePath@data[$n] -keygen`;

    @icua = split( ' = ', $icu[2] );
    @icub = split( ' ',   $icua[1] );
    @nixa = split( ' = ', $nix[2] );
    @nixb = split( ' ',   $nixa[1] );

    $resultICU = $resultICU.$icub[0].",";
    $resultNIX = $resultNIX.$nixb[0].",";

    #keygen len
    @icua = split( ' = ', $icu[3] );
    @nixa = split( ' = ', $nix[3] );

    chomp( @icua[1] );
    chomp( @nixa[1] );

    $resultICU = $resultICU.$icua[1].",";
    $resultNIX = $resultNIX.$nixa[1].",";

    my @resultSplitICU;
    my @resultSplitNIX;

    #percent
    for ( $i = 0 ; $i < 3 ; $i++ ) {
        my $percent = 0;
        @resultSplitICU = split( ',', $resultICU );
        @resultSplitNIX = split( ',', $resultNIX );
        if ( @resultSplitICU[ 2 + $i ] > 0 ) {
            $percent = substr((((
                @resultSplitNIX[ 2 + $i ] - @resultSplitICU[ 2 + $i ]) / @resultSplitICU[ 2 + $i ]) * 100),
                0, 7);
        }
        @resultPER[$n] = @resultPER[$n].$percent."%,";
    }

    #store ICU result
    for ( $j = 0 ; $j < 3 ; $j++ ) {
        @resultFIN[$n] = @resultFIN[$n].@resultSplitICU[ 2 + $j ].",";
    }

    #store Unix result
    for ( $j = 0 ; $j < 3 ; $j++ ) {
        @resultFIN[$n] = @resultFIN[$n].@resultSplitNIX[ 2 + $j ].",";
    }

    #store Percent result
    @resultFIN[$n] = @resultFIN[$n].@resultPER[$n];
}

# Print the results in a HTML page
printOutput();

exit(0);

# This subroutine creates the web page and prints out the results in a table
sub printOutput {
    my $title = "Collation:  ICU " . $ICULatestVersion . " vs GLIBC";
    my $html  = localtime;
    $html =~ s/://g;         # ':' illegal
    $html =~ s/\s*\d+$//;    # delete year
    $html =~ s/^\w+\s*//;    # delete dow
    $html = "CollationPerformance $html.html";
    $html = "../results/" . $html;
    $html =~ s/ /_/g;
    open( HTML, ">$html" ) or die "Can't write to $html: $!";
    print HTML <<EOF;
﻿<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<title>Collation: ICU4C vs. glibc</title>
<link rel="stylesheet" href="../icu.css" type="text/css" />
</head>
<body>
<!--#include virtual="../ssi/header.html" -->
EOF

    print HTML "<h2>Collation: ICU4C ".$ICULatestVersion." vs. GLIBC</h2>\n";

    print HTML <<EOF;
<p>The performance test takes a locale and creates a RuleBasedCollator with
default options. A large list of names is used as data in each test, where the
names vary according to language. Each Collation operation over the whole list
is repeated 1000 times. The percentage values in the final column are the most
useful. They measure differences, where positive is better for ICU4C, and
negative is better for the compared implementation.</p>
<h3>Key</h3>
<table border="1" cellspacing="0" cellpadding="4">
<tr>
<th align="left">Operation</th>
<th align="left">Units</th>
<th align="left">Description</th>
</tr>
<tr>
<td>strcoll</td>
<td>nanosecs</td>
<td>Timing for string collation, an incremental compare of strings.</td>
</tr>
<tr>
<td>keygen</td>
<td>nanosecs</td>
<td>Timing for generation of sort keys, used to 'precompile' information so
that subsequent operations can use binary comparison.</td>
</tr>
<tr>
<td>keylen</td>
<td>bytes/char</td>
<td>The average length of the generated sort keys, in bytes per character
(Unicode/ISO 10646 code point). Generally this is the important field for sort
key performance, since it directly impacts the time necessary for binary
comparison, and the overhead of memory usage and retrieval time for sort
keys.</td>
</tr>
</table>
EOF
    printData();
    
    print HTML <<EOF;
<h3><i>Notes</i></h3>
<ol>
<li>As with all performance measurements, the results will vary according to
the hardware and compiler. The strcoll operation is particularly sensitive; we
have found that even slight changes in code alignment can produce 10%
differences.</li>
<li>For more information on incremental vs. sort key comparison, the importance
of multi-level sorting, and other features of collation, see <a href=
"http://www.unicode.org/reports/tr10/">Unicode Collation (UCA)</a>.</li>
<li>For general information on ICU collation see <a href=
"/userguide/Collate_Intro.html">User Guide</a>.</li>
<li>For information on APIs, see <a href="/apiref/icu4c/ucol_8h.html">C</a>,
<a href="/apiref/icu4c/classCollator.html">C++</a>, or <a href=
"/apiref/icu4j/com/ibm/icu/text/Collator.html">Java</a>.</li>
</ol>
<!--#include virtual="../ssi/footer.html" -->
</body>
</html>

EOF

    close(HTML) or die "Can't close $html: $!";
}

# This subroutine formats and prints the table.
sub printData() {
    print HTML <<EOF;
<h3>Data</h3>
<table border="1" cellspacing="0" cellpadding="4">
<tr>
<td align="left"><b>Locale</b></td>
<td align="left"><b>Data file</b></td>
<td align="left"><b>strcoll</b> <i>(ICU)</i></td>
<td align="left"><b>keygen</b> <i>(ICU)</i></td>
<td align="left"><b>keylen</b> <i>(ICU)</i></td>
<td align="left"><b>strcoll</b> <i>(GLIBC)</i></td>
<td align="left"><b>keygen</b> <i>(GLIBC)</i></td>
<td align="left"><b>keylen</b> <i>(GLIBC)</i></td>
<td align="left"><b>strcoll</b> <i>(GLIBC-ICU)/ICU)</i></td>
<td align="left"><b>keygen</b> <i>(GLIBC-ICU)/ICU)</i></td>
<td align="left"><b>keylen</b> <i>(GLIBC-ICU)/ICU)</i></td>
</tr>
EOF

    for ( $n = 0 ; $n < @resultFIN ; $n++ ) {
        print HTML "<tr>";
        my @parsed = split( ',', @resultFIN[$n] );
        for ( $i = 0 ; $i < @parsed ; $i++ ) {
            my $value = @parsed[$i];
            print HTML "<td align=\"center\">";

            if ( $value =~ m/^[-]/ ) {
                print HTML "<font color=\"red\">$value</font>";
            }
            else {
                print HTML "$value";
            }

            print HTML "</td>";

        }
        print HTML "</tr>\n";
    }

    print HTML<<EOF;
</table>
EOF
}
