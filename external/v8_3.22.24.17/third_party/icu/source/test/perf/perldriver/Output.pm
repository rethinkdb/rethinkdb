#!/usr/local/bin/perl

#  ********************************************************************
#  * COPYRIGHT:
#  * Copyright (c) 2002, International Business Machines Corporation and
#  * others. All Rights Reserved.
#  ********************************************************************


use strict;

use Dataset;

my $TABLEATTR = 'BORDER="1" CELLPADDING="4" CELLSPACING="0"';
my $outType = "HTML";
my $html = "noName";
my $inTable;
my @headers;
my @timetypes = ("mean per op", "error per op", "events", "per event");
my %raw;
my $current = "";
my $exp = 0;
my $mult = 1e9; #use nanoseconds
my $perc = 100; #for percent
my $printEvents = 0;
my $legend = "<a name=\"Legend\">\n<h2>Table legend</h2></a><ul>";
my $legendDone = 0;
my %options;
my $operationIs = "operation";
my $eventIs = "event";

sub startTest {
  $current = shift;
  $exp = 0;
  outputData($current);
}

sub printLeg {
  if(!$legendDone) {
    my $message;
    foreach $message (@_) {
      $legend .= "<li>".$message."</li>\n";
    }
  }
}

sub outputDist {
  my $value = shift;
  my $percent = shift;
  my $mean = $value->getMean;
  my $error = $value->getError;
  print HTML "<td class=\"";
  if($mean > 0) {
    print HTML "value";
  } else {
    print HTML "worse";
  }
  print HTML "\">";
  if($percent) {
    print HTML formatPercent(2, $mean);
  } else {
    print HTML formatNumber(2, $mult, $mean);
  }
  print HTML "</td>\n";  
  print HTML "<td class=\"";
  if((($error*$mult < 10)&&!$percent) || (($error<10)&&$percent)) {
    print HTML "error";
  } else {
    print HTML "errorLarge";
  }
  print HTML "\">&plusmn;";
  if($percent) {
    print HTML formatPercent(2, $error);
  } else {
    print HTML formatNumber(2, $mult, $error);
  }
  print HTML "</td>\n";  
}
  
sub outputValue {
  my $value = shift;
  print HTML "<td class=\"sepvalue\">";
  print HTML $value;
  #print HTML formatNumber(2, 1, $value);
  print HTML "</td>\n";  
}

sub startTable {
  #my $printEvents = shift;
  $inTable = 1;
  my $i;
  print HTML "<table $TABLEATTR>\n";
  print HTML "<tbody>\n";
  if($#headers >= 0) {
    my ($header, $i);
    print HTML "<tr>\n";
    print HTML "<th rowspan=\"2\" class=\"testNameHeader\"><a href=\"#TestName\">Test Name</a></th>\n";
    print HTML "<th rowspan=\"2\" class=\"testNameHeader\"><a href=\"#Ops\">Ops</a></th>\n";
    printLeg("<a name=\"Test Name\">TestName</a> - name of the test as set by the test writer\n", "<a name=\"Ops\">Ops</a> - number of ".$operationIs."s per iteration\n");
    if(!$printEvents) {
      print HTML "<th colspan=".((4*($#headers+1))-2)." class=\"sourceType\">Per Operation</th>\n";
    } else {
      print HTML "<th colspan=".((2*($#headers+1))-2)." class=\"sourceType\">Per Operation</th>\n";
      print HTML "<th colspan=".((5*($#headers+1))-2)." class=\"sourceType\">Per Event</th>\n";
    }
    print HTML "</tr>\n<tr>\n";
    if(!$printEvents) {
      foreach $header (@headers) {
	print HTML "<th class=\"source\" colspan=2><a href=\"#meanop_$header\">$header<br>/op</a></th>\n";
	printLeg("<a name=\"meanop_$header\">$header /op</a> - mean time and error for $header per $operationIs");
      }
    }
    for $i (1 .. $#headers) {
      print HTML "<th class=\"source\" colspan=2><a href=\"#mean_op_$i\">ratio $i<br>/op</a></th>\n";
      printLeg("<a name=\"mean_op_$i\">ratio $i /op</a> - ratio and error of per $operationIs time, calculated as: (($headers[0] - $headers[$i])/$headers[$i])*100%, mean value");
    }
    if($printEvents) {
      foreach $header (@headers) {
	print HTML "<th class=\"source\"><a href=\"#events_$header\">$header<br>events</a></th>\n";
	printLeg("<a name=\"events_$header\">$header events</a> - number of ".$eventIs."s for $header per iteration");
      }
      foreach $header (@headers) {
	print HTML "<th class=\"source\" colspan=2><a href=\"#mean_ev_$header\">$header<br>/ev</a></th>\n";
	printLeg("<a name=\"mean_ev_$header\">$header /ev</a> - mean time and error for $header per $eventIs");
      }
      for $i (1 .. $#headers) {
	print HTML "<th class=\"source\" colspan=2><a href=\"#mean_ev_$i\">ratio $i<br>/ev</a></th>\n";
	printLeg("<a name=\"mean_ev_$i\">ratio $i /ev</a> - ratio and error of per $eventIs time, calculated as: (($headers[0] - $headers[$i])/$headers[$i])*100%, mean value");
      }
    }
    print HTML "</tr>\n";
  }
  $legendDone = 1;
}

sub closeTable {
  if($inTable) {
    undef $inTable;
    print HTML "</tr>\n";
    print HTML "</tbody>";
    print HTML "</table>\n";
  }
}

sub newRow {
  if(!$inTable) {
    startTable;
  } else {
    print HTML "</tr>\n";
  }
  print HTML "<tr>";
}

sub outputData {
  if($inTable) {
    my $msg = shift;
    my $align = shift;
    print HTML "<td";
    if($align) {
      print HTML " align = $align>";
    } else {
      print HTML ">";
    }
    print HTML "$msg";
    print HTML "</td>";
  } else {
    my $message;
    foreach $message (@_) {
      print HTML "$message";
    }
  }
}

sub setupOutput {
  my $date = localtime;
  my $options = shift;
  %options = %{ $options };
  my $title = $options{ "title" };
  my $headers = $options{ "headers" };
  if($options{ "operationIs" }) {
    $operationIs = $options{ "operationIs" };
  }
  if($options{ "eventIs" }) {
    $eventIs = $options{ "eventIs" };
  }
  @headers = split(/ /, $headers);
  my ($t, $rest);
  ($t, $rest) = split(/\.\w+/, $0);
  $t =~ /^.*\W(\w+)$/;
  $t = $1;
  if($outType eq 'HTML') {
    $html = $date;
    $html =~ s/://g; # ':' illegal
    $html =~ s/\s*\d+$//; # delete year
    $html =~ s/^\w+\s*//; # delete dow
    $html = "$t $html.html";
    if($options{ "outputDir" }) {
      $html = $options{ "outputDir" }."/".$html;
    }
    $html =~ s/ /_/g;

    open(HTML,">$html") or die "Can't write to $html: $!";

#<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
    print HTML <<EOF;
<HTML>
   <HEAD>
   <meta http-equiv="Content-Type" content="text/html; charset=utf-8">
      <TITLE>$title</TITLE>
<style>
<!--
body         { font-size: 10pt; font-family: sans-serif }
th           { font-size: 10pt; border: 0 solid #000080; padding: 5 }
th.testNameHeader { border-width: 1 }
th.testName  { text-align: left; border-left-width: 1; border-right-width: 1; 
               border-bottom-width: 1 }
th.source    { border-right-width: 1; border-bottom-width: 1 }
th.sourceType { border-right-width: 1; border-top-width: 1; border-bottom-width: 1 }
td           { font-size: 10pt; text-align: Right; border: 0 solid #000080; padding: 5 }
td.string    { text-align: Left; border-bottom-width:1; border-right-width:1 }
td.sepvalue  { border-bottom-width: 1; border-right-width: 1 }
td.value     { border-bottom-width: 1 }
td.worse     { color: #FF0000; font-weight: bold; border-bottom-width: 1 }
td.error     { font-size: 75%; border-right-width: 1; border-bottom-width: 1 }
td.errorLarge { font-size: 75%; color: #FF0000; font-weight: bold; border-right-width: 1; 
               border-bottom-width: 1 }
A:link    { color: black; font-weight: normal; text-decoration: none}    /* unvisited links */
A:visited { color: blue; font-weight: normal; text-decoration: none }   /* visited links   */
A:hover   { color: red; font-weight: normal; text-decoration: none } /* user hovers     */
A:active  { color: lime; font-weight: normal; text-decoration: none }   /* active links    */
-->
</style>
   </HEAD>
   <BODY bgcolor="#FFFFFF" LINK="#006666" VLINK="#000000">
EOF
    print HTML "<H1>$title</H1>\n";

    #print HTML "<H2>$TESTCLASS</H2>\n";
  }
}

sub closeOutput {
  if($outType eq 'HTML') {
    if($inTable) {
      closeTable;
    }
    $legend .= "</ul>\n";
    print HTML $legend;
    outputRaw();
    print HTML <<EOF;
   </BODY>
</HTML>
EOF
    close(HTML) or die "Can't close $html: $!";
  }
}


sub outputRaw {
  print HTML "<h2>Raw data</h2>";
  my $key;
  my $i;
  my $j;
  my $k;
  print HTML "<table $TABLEATTR>\n";
  for $key (sort keys %raw) {
    my $printkey = $key;
    $printkey =~ s/\<br\>/ /g;
    if($printEvents) {
      if($key ne "") {
	print HTML "<tr><th class=\"testNameHeader\" colspan = 7>$printkey</td></tr>\n"; # locale and data file
      }
      print HTML "<tr><th class=\"testName\">test name</th><th class=\"testName\">interesting arguments</th><th class=\"testName\">iterations</th><th class=\"testName\">operations</th><th class=\"testName\">mean time (ns)</th><th class=\"testName\">error (ns)</th><th class=\"testName\">events</th></tr>\n";
    } else {
      if($key ne "") {
	print HTML "<tr><th class=\"testName\" colspan = 6>$printkey</td></tr>\n"; # locale and data file
      }
      print HTML "<tr><th class=\"testName\">test name</th><th class=\"testName\">interesting arguments</th><th class=\"testName\">iterations</th><th class=\"testName\">operations</th><th class=\"testName\">mean time (ns)</th><th class=\"testName\">error (ns)</th></tr>\n";
    }
    $printkey =~ s/[\<\>\/ ]//g;
      
    my %done;
    for $i ( $raw{$key} ) {
      print HTML "<tr>";
      for $j ( @$i ) {
	my ($test, $args);
	($test, $args) = split(/,/, shift(@$j));

	print HTML "<th class=\"testName\">";
	if(!$done{$test}) {
	  print HTML "<a name=\"".$printkey."_".$test."\">".$test."</a>";
	  $done{$test} = 1;
	} else {
	  print HTML $test;
	}
	print HTML "</th>";

	print HTML "<td class=\"string\">".$args."</td>";
	
	print HTML "<td class=\"sepvalue\">".shift(@$j)."</td>";
	print HTML "<td class=\"sepvalue\">".shift(@$j)."</td>";

	my @data = @{ shift(@$j) };
	my $ds = Dataset->new(@data);
	print HTML "<td class=\"sepvalue\">".formatNumber(4, $mult, $ds->getMean)."</td><td class=\"sepvalue\">".formatNumber(4, $mult, $ds->getError)."</td>";
	if($#{ $j } >= 0) {
	  print HTML "<td class=\"sepvalue\">".shift(@$j)."</td>";
	}
	print HTML "</tr>\n";
      }
    }
  }
}

sub store {
  $raw{$current}[$exp++] = [@_];
}

sub outputRow {
  #$raw{$current}[$exp++] =  [@_];
  my $testName = shift;
  my @iterPerPass = @{shift(@_)};
  my @noopers =  @{shift(@_)};
   my @timedata =  @{shift(@_)};
  my @noevents;
  if($#_ >= 0) {
    @noevents =  @{shift(@_)};
  }
  if(!$inTable) {
    if(@noevents) {
      $printEvents = 1;
      startTable;
    } else {
      startTable;
    }
  }
  debug("No events: @noevents, $#noevents\n");

  my $j;
  my $loc = $current;
  $loc =~ s/\<br\>/ /g;
  $loc =~ s/[\<\>\/ ]//g;

  # Finished one row of results. Outputting
  newRow;
  #outputData($testName, "LEFT");
  print HTML "<th class=\"testName\"><a href=\"#".$loc."_".$testName."\">$testName</a></th>\n";
  #outputData($iterCount);
  #outputData($noopers[0], "RIGHT");
  outputValue($noopers[0]);

  if(!$printEvents) {
    for $j ( 0 .. $#timedata ) {
      my $perOperation = $timedata[$j]->divideByScalar($iterPerPass[$j]*$noopers[$j]); # time per operation
      #debug("Time per operation: ".formatSeconds(4, $perOperation->getMean, $perOperation->getError)."\n");
      outputDist($perOperation);
    }
  }
  my $baseLinePO = $timedata[0]->divideByScalar($iterPerPass[0]*$noopers[0]);
  for $j ( 1 .. $#timedata ) {
    my $perOperation = $timedata[$j]->divideByScalar($iterPerPass[$j]*$noopers[$j]); # time per operation
    my $ratio = $baseLinePO->subtract($perOperation);
    $ratio = $ratio->divide($perOperation);
    outputDist($ratio, "%");
  }   
  if (@noevents) {
    for $j ( 0 .. $#timedata ) {
      #outputData($noevents[$j], "RIGHT");
      outputValue($noevents[$j]);
    }
    for $j ( 0 .. $#timedata ) {
      my $perEvent =  $timedata[$j]->divideByScalar($iterPerPass[$j]*$noevents[$j]); # time per event
      #debug("Time per operation: ".formatSeconds(4, $perEvent->getMean, $perEvent->getError)."\n");
      outputDist($perEvent);
    }   
    my $baseLinePO = $timedata[0]->divideByScalar($iterPerPass[0]*$noevents[0]);
    for $j ( 1 .. $#timedata ) {
      my $perOperation = $timedata[$j]->divideByScalar($iterPerPass[$j]*$noevents[$j]); # time per operation
      my $ratio = $baseLinePO->subtract($perOperation);
      $ratio = $ratio->divide($perOperation);
      outputDist($ratio, "%");
    }   
  }
}


1;

#eof
