# *   Copyright (C) 2003-2010, International Business Machines
# *   Corporation and others.  All Rights Reserved.
# A list of txt's to build
# Note: 
#
#   If you are thinking of modifying this file, READ THIS. 
#
# Instead of changing this file [unless you want to check it back in],
# you should consider creating a 'misclocal.mk' file in this same directory.
# Then, you can have your local changes remain even if you upgrade or re-
# configure ICU.
#
# Example 'misclocal.mk' files:
#
#  * To add an additional file to the list: 
#    _____________________________________________________
#    |  MISC_SOURCE_LOCAL =   myFile.txt ...
#
#  * To REPLACE the default list and only build a subset of files:
#    _____________________________________________________
#    |  MISC_SOURCE = zoneinfo.txt
#
#

MISC_SOURCE = \
zoneinfo64.txt supplementalData.txt likelySubtags.txt plurals.txt numberingSystems.txt icuver.txt icustd.txt metaZones.txt windowsZones.txt keyTypeData.txt timezoneTypes.txt
