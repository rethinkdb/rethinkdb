/*
*******************************************************************************
*   Copyright (C) 2004-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*******************************************************************************
*
*   file name:  udraft.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   Created by: genheaders.pl, a perl script written by Ram Viswanadha
*
*  Contains data for commenting out APIs.
*  Gets included by umachine.h
*
*  THIS FILE IS MACHINE-GENERATED, DON'T PLAY WITH IT IF YOU DON'T KNOW WHAT
*  YOU ARE DOING, OTHERWISE VERY BAD THINGS WILL HAPPEN!
*/

#ifndef UDRAFT_H
#define UDRAFT_H

#ifdef U_HIDE_DRAFT_API

#    if U_DISABLE_RENAMING
#        define ubidi_getBaseDirection ubidi_getBaseDirection_DRAFT_API_DO_NOT_USE
#        define uidna_close uidna_close_DRAFT_API_DO_NOT_USE
#        define uidna_labelToASCII uidna_labelToASCII_DRAFT_API_DO_NOT_USE
#        define uidna_labelToASCII_UTF8 uidna_labelToASCII_UTF8_DRAFT_API_DO_NOT_USE
#        define uidna_labelToUnicode uidna_labelToUnicode_DRAFT_API_DO_NOT_USE
#        define uidna_labelToUnicodeUTF8 uidna_labelToUnicodeUTF8_DRAFT_API_DO_NOT_USE
#        define uidna_nameToASCII uidna_nameToASCII_DRAFT_API_DO_NOT_USE
#        define uidna_nameToASCII_UTF8 uidna_nameToASCII_UTF8_DRAFT_API_DO_NOT_USE
#        define uidna_nameToUnicode uidna_nameToUnicode_DRAFT_API_DO_NOT_USE
#        define uidna_nameToUnicodeUTF8 uidna_nameToUnicodeUTF8_DRAFT_API_DO_NOT_USE
#        define uidna_openUTS46 uidna_openUTS46_DRAFT_API_DO_NOT_USE
#        define uloc_forLanguageTag uloc_forLanguageTag_DRAFT_API_DO_NOT_USE
#        define uloc_toLanguageTag uloc_toLanguageTag_DRAFT_API_DO_NOT_USE
#        define unorm2_getDecomposition unorm2_getDecomposition_DRAFT_API_DO_NOT_USE
#        define uregex_end64 uregex_end64_DRAFT_API_DO_NOT_USE
#        define uregex_find64 uregex_find64_DRAFT_API_DO_NOT_USE
#        define uregex_getFindProgressCallback uregex_getFindProgressCallback_DRAFT_API_DO_NOT_USE
#        define uregex_lookingAt64 uregex_lookingAt64_DRAFT_API_DO_NOT_USE
#        define uregex_matches64 uregex_matches64_DRAFT_API_DO_NOT_USE
#        define uregex_patternUText uregex_patternUText_DRAFT_API_DO_NOT_USE
#        define uregex_regionEnd64 uregex_regionEnd64_DRAFT_API_DO_NOT_USE
#        define uregex_regionStart64 uregex_regionStart64_DRAFT_API_DO_NOT_USE
#        define uregex_reset64 uregex_reset64_DRAFT_API_DO_NOT_USE
#        define uregex_setFindProgressCallback uregex_setFindProgressCallback_DRAFT_API_DO_NOT_USE
#        define uregex_setRegion64 uregex_setRegion64_DRAFT_API_DO_NOT_USE
#        define uregex_setRegionAndStart uregex_setRegionAndStart_DRAFT_API_DO_NOT_USE
#        define uregex_start64 uregex_start64_DRAFT_API_DO_NOT_USE
#        define uscript_getScriptExtensions uscript_getScriptExtensions_DRAFT_API_DO_NOT_USE
#        define uscript_hasScript uscript_hasScript_DRAFT_API_DO_NOT_USE
#    else
#        define ubidi_getBaseDirection_4_6 ubidi_getBaseDirection_DRAFT_API_DO_NOT_USE
#        define uidna_close_4_6 uidna_close_DRAFT_API_DO_NOT_USE
#        define uidna_labelToASCII_4_6 uidna_labelToASCII_DRAFT_API_DO_NOT_USE
#        define uidna_labelToASCII_UTF8_4_6 uidna_labelToASCII_UTF8_DRAFT_API_DO_NOT_USE
#        define uidna_labelToUnicodeUTF8_4_6 uidna_labelToUnicodeUTF8_DRAFT_API_DO_NOT_USE
#        define uidna_labelToUnicode_4_6 uidna_labelToUnicode_DRAFT_API_DO_NOT_USE
#        define uidna_nameToASCII_4_6 uidna_nameToASCII_DRAFT_API_DO_NOT_USE
#        define uidna_nameToASCII_UTF8_4_6 uidna_nameToASCII_UTF8_DRAFT_API_DO_NOT_USE
#        define uidna_nameToUnicodeUTF8_4_6 uidna_nameToUnicodeUTF8_DRAFT_API_DO_NOT_USE
#        define uidna_nameToUnicode_4_6 uidna_nameToUnicode_DRAFT_API_DO_NOT_USE
#        define uidna_openUTS46_4_6 uidna_openUTS46_DRAFT_API_DO_NOT_USE
#        define uloc_forLanguageTag_4_6 uloc_forLanguageTag_DRAFT_API_DO_NOT_USE
#        define uloc_toLanguageTag_4_6 uloc_toLanguageTag_DRAFT_API_DO_NOT_USE
#        define unorm2_getDecomposition_4_6 unorm2_getDecomposition_DRAFT_API_DO_NOT_USE
#        define uregex_end64_4_6 uregex_end64_DRAFT_API_DO_NOT_USE
#        define uregex_find64_4_6 uregex_find64_DRAFT_API_DO_NOT_USE
#        define uregex_getFindProgressCallback_4_6 uregex_getFindProgressCallback_DRAFT_API_DO_NOT_USE
#        define uregex_lookingAt64_4_6 uregex_lookingAt64_DRAFT_API_DO_NOT_USE
#        define uregex_matches64_4_6 uregex_matches64_DRAFT_API_DO_NOT_USE
#        define uregex_patternUText_4_6 uregex_patternUText_DRAFT_API_DO_NOT_USE
#        define uregex_regionEnd64_4_6 uregex_regionEnd64_DRAFT_API_DO_NOT_USE
#        define uregex_regionStart64_4_6 uregex_regionStart64_DRAFT_API_DO_NOT_USE
#        define uregex_reset64_4_6 uregex_reset64_DRAFT_API_DO_NOT_USE
#        define uregex_setFindProgressCallback_4_6 uregex_setFindProgressCallback_DRAFT_API_DO_NOT_USE
#        define uregex_setRegion64_4_6 uregex_setRegion64_DRAFT_API_DO_NOT_USE
#        define uregex_setRegionAndStart_4_6 uregex_setRegionAndStart_DRAFT_API_DO_NOT_USE
#        define uregex_start64_4_6 uregex_start64_DRAFT_API_DO_NOT_USE
#        define uscript_getScriptExtensions_4_6 uscript_getScriptExtensions_DRAFT_API_DO_NOT_USE
#        define uscript_hasScript_4_6 uscript_hasScript_DRAFT_API_DO_NOT_USE
#    endif /* U_DISABLE_RENAMING */

#endif /* U_HIDE_DRAFT_API */
#endif /* UDRAFT_H */

