# *   Copyright (C) 1998-2010, International Business Machines
# *   Corporation and others.  All Rights Reserved.
RBNF_CLDR_VERSION = 1.9
# A list of txt's to build
# Note:
#
#
#  * To add an additional locale to the list:
#    _____________________________________________________
#    |  RBNF_SOURCE_LOCAL =   myLocale.txt ...
#
#  * To REPLACE the default list and only build with a few
#    locales:
#    _____________________________________________________
#    |  RBNF_SOURCE = ar.txt ar_AE.txt en.txt de.txt zh.txt
#
# Chrome note: RBNF is not used by Chrome/Webkit. So, we don't include them.

# Aliases without a corresponding xx.xml file (see icu-config.xml & build.xml)
RBNF_SYNTHETIC_ALIAS =


# All aliases (to not be included under 'installed'), but not including root.
RBNF_ALIAS_SOURCE = $(RBNF_SYNTHETIC_ALIAS)


# Ordinary resources
RBNF_SOURCE =
