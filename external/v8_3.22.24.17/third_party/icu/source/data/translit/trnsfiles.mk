# *   Copyright (C) 1997-2006, International Business Machines
# *   Corporation and others.  All Rights Reserved.
# A list of txt's to build
# Note: 
#
#   If you are thinking of modifying this file, READ THIS. 
#
# Instead of changing this file [unless you want to check it back in],
# you should consider creating a 'trnslocal.mk' file in this same directory.
# Then, you can have your local changes remain even if you upgrade or re
# configure the ICU.
#
# Example 'trnslocal.mk' files:
#
#  * To add an additional transliterators to the list: 
#    _____________________________________________________
#    |  TRANSLIT_SOURCE_LOCAL =   myTranslitRules.txt ...
#
#  * To REPLACE the default list and only build with a few
#     transliterators:
#    _____________________________________________________
#    |  TRANLIST_SOURCE = el.txt th.txt
#
#

TRANSLIT_SOURCE=root.txt en.txt el.txt
