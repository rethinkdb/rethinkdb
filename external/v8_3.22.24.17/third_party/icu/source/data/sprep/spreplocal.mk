# *   Copyright (C) 2011, International Business Machines
# *   Corporation and others.  All Rights Reserved.
# A list of txt's to build
#
#  * To add an additional locale to the list: 
#    _____________________________________________________
#    |  SPREP_SOURCE_LOCAL =   myStringPrep.txt ...
#
#  * To REPLACE the default list and only build a subset of files:
#    _____________________________________________________
#    |  SPREP_SOURCE = rfc4518.txt
#
# Chromium: Drop all other stringprep files other than RFC 3491 for IDNA 2003. ICU 4.6
# supports UTS 46 (IDNA2003, transition, and IDNA 2008).
# If we ever need to support other stringprep (e.g. LDAP, XMPP, SASL), we have to add back
# the corresponding prep files.
# rfc3491 : stringprep profile for IDN 2003
# rfc3530 : NFS
# rfc3722 : String Profile for Internet Small Computer 
#           Systems Interface (iSCSI) Names
# rfc3920 : XMPP (Extensible Messaging and Presence Protocol)
# rfc4011 : Policy Based Management Information Base, SNMP
# rfc4013 : SASLprep: Stringprep Profile for User Names and Passwords
# rfc4505 :  Anonymous Simple Authentication and Security Layer (SASL) Mechanism
# rfc4518 : LDAP  Internationalized String Preparation
#
SPREP_SOURCE = rfc3491.txt
