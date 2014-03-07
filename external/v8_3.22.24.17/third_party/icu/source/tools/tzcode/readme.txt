**********************************************************************
* Copyright (c) 2003-2007, International Business Machines
* Corporation and others.  All Rights Reserved.
**********************************************************************
* Author: Alan Liu
* Created: August 18 2003
* Since: ICU 2.8
**********************************************************************

Note:  this directory currently contains tzcode as of tzcode2006h.tar.gz
   with localtime.c  patches from tzcode2006i.tar.gz


----------------------------------------------------------------------
OVERVIEW

This file describes the tools in icu/source/tools/tzcode

The purpose of these tools is to process the zoneinfo or "Olson" time
zone database into a form usable by ICU4C (release 2.8 and later).
Unlike earlier releases, ICU4C 2.8 supports historical time zone
behavior, as well as the full set of Olson compatibility IDs.

References:

ICU4C:  http://www.icu-project.org/
Olson:  ftp://elsie.nci.nih.gov/pub/

----------------------------------------------------------------------
ICU4C vs. ICU4J

For ICU releases >= 2.8, both ICU4C and ICU4J implement full
historical time zones, based on Olson data.  The implementations in C
and Java are somewhat different.  The C implementation is a
self-contained implementation, whereas ICU4J uses the underlying JDK
1.3 or 1.4 time zone implementation.

Older versions of ICU (C and Java <= 2.6) implement a "present day
snapshot".  This only reflects current time zone behavior, without
historical variation.  Furthermore, it lacks the full set of Olson
compatibility IDs.

----------------------------------------------------------------------
BACKGROUND

The zoneinfo or "Olson" time zone package is used by various systems
to describe the behavior of time zones.  The package consists of
several parts.  E.g.:

  Index of ftp://elsie.nci.nih.gov/pub/

  classictzcode.tar.gz    65 KB        12/10/1994    12:00:00 AM
  classictzdata.tar.gz    67 KB        12/10/1994    12:00:00 AM
  e5+57.tar.gz            2909 KB      3/22/1993     12:00:00 AM
  iso8601.ps.gz           16 KB        7/27/1996     12:00:00 AM
  leastsq.xls             49 KB        4/24/1997     12:00:00 AM
  ltroff.tar.gz           36 KB        7/16/1993     12:00:00 AM
  pi.shar.gz              4 KB         3/9/1994      12:00:00 AM
  tzarchive.gz            3412 KB      8/18/2003     4:00:00 AM
  tzcode2003a.tar.gz      98 KB        3/24/2003     2:32:00 PM
  tzdata2003a.tar.gz      132 KB       3/24/2003     2:32:00 PM

ICU only uses the tzdataYYYYV.tar.gz files,
where YYYY is the year and V is the version letter ('a'...'z').

This directory has partial contents of tzcode checked into ICU

----------------------------------------------------------------------
HOWTO

0. Note, these instructions will only work on POSIX type systems.

1. Obtain the current versions of tzdataYYYYV.tar.gz (aka `tzdata') from
   the FTP site given above.  Either manually download or use wget:

   $ cd {path_to}/icu/source/tools/tzcode
   $ wget "ftp://elsie.nci.nih.gov/pub/tzdata*.tar.gz"

2. Copy only one tzdata*.tar.gz file into the icu/source/tools/tzcode/
   directory (this directory).

   *** Make sure you only have ONE FILE named tzdata*.tar.gz in the
       directory.

3. Build ICU normally. You will see a notice "updating zoneinfo.txt..."

### Following instructions for ICU maintainers only ###

4. Obtain the current version of tzcodeYYYY.tar.gz from the FTP site to
   this directory.

5. Run make target "check-dump".  This target extract makes the original
   tzcode and compile the original tzdata with icu supplemental data
   (icuzones).  Then it makes zdump / icuzdump and dump all time
   transitions for all ICU timezone to files under zdumpout / icuzdumpout
   directory.  When they produce different results, the target returns
   the error.

6. Don't forget to check in the new zoneinfo.txt (from its location at
   {path_to}/icu/source/data/misc/zoneinfo.txt) into SVN.

