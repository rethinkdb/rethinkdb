#!/bin/sh
# This fetches current versions of the included dependencies .
for itemf in google-closure-compiler google-closure-library protobuf-plugin-closure ;
do
	if [ -e "$itemf" ] ; then
		rm -rf "$itemf" ;
	fi ;
done ;

