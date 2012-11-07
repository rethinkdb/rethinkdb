#!/bin/sh
# Copyright 2010-2012 RethinkDB, all rights reserved.
# This script is for generating a changelog file for the Debian source package. git-dch is problematic, so we generate a stub changelog each time.
# The script omits automatic base-finding and presently expects to be run from the root of the repository.
PRODBASE=. ;
PRODUCTNAME="rethinkdb" ;
PRODUCTVERSION="`"$PRODBASE"/scripts/gen-version.sh`""-0"
if [ "$UBRELEASE" != "" ] ;
then
	PRODUCTVERSION="$PRODUCTVERSION"ubuntu1~"$UBRELEASE" ;
	OSRELEASE="$UBRELEASE" ;
else
	OSRELEASE="unstable" ;
fi ;
# UBRELEASE="precise" ;
# We now provide UBRELEASE in the environment .
TIMESTAMPTIME="`date "+%a, %d %b %Y %H:%M:%S"`" ;
TIMESTAMPOFFSET="-0800" ;
TIMESTAMPFULL="$TIMESTAMPTIME"" ""$TIMESTAMPOFFSET" ;
AGENTNAME="RethinkDB Packaging"
AGENTMAIL="packaging@rethinkdb.com"
CHANGELOGFILE="$PRODBASE"/"debian/changelog" ;
echo "$PRODUCTNAME"" (""$PRODUCTVERSION"") ""$UBRELEASE""; urgency=low" > "$CHANGELOGFILE" ;
echo "" >> "$CHANGELOGFILE" ;
echo "  * Release." >> "$CHANGELOGFILE" ;
echo "" >> "$CHANGELOGFILE" ;
echo " -- ""$AGENTNAME"" <""$AGENTMAIL"">  ""$TIMESTAMPFULL" >> "$CHANGELOGFILE" ;

