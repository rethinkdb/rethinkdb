#!/bin/sh
# Copyright 2010-2012 RethinkDB, all rights reserved.
# This script is for generating a changelog file for the Debian source package. git-dch is problematic, so we generate a stub changelog each time.
# The script omits automatic base-finding and presently expects to be run from the root of the repository.
# Variables expected in the environment include
#   PRODUCT_NAME
#   PRODUCT_VERSION
#   OS_RELEASE
# .

TIMESTAMP_TIME="`date "+%a, %d %b %Y %H:%M:%S"`" ;
TIMESTAMP_OFFSET="-0800" ;
TIMESTAMP_FULL="$TIMESTAMP_TIME"" ""$TIMESTAMP_OFFSET" ;
AGENT_NAME="RethinkDB Packaging"
AGENT_MAIL="packaging@rethinkdb.com"

echo "$PRODUCT_NAME ($PRODUCT_VERSION) $OS_RELEASE; urgency=low"
echo
echo "  * Release."
echo
echo " -- $AGENT_NAME <$AGENT_MAIL>  $TIMESTAMP_FULL"
# Note that there are two spaces between the e-mail address and the time-stamp. This was no accident.

