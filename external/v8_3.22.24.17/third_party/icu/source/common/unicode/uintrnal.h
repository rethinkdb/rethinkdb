/*
*******************************************************************************
*   Copyright (C) 2004-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*******************************************************************************
*
*   file name:  uintrnal.h
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

#ifndef UINTRNAL_H
#define UINTRNAL_H

#ifdef U_HIDE_INTERNAL_API

#    if U_DISABLE_RENAMING
#        define RegexPatternDump RegexPatternDump_INTERNAL_API_DO_NOT_USE
#        define bms_close bms_close_INTERNAL_API_DO_NOT_USE
#        define bms_empty bms_empty_INTERNAL_API_DO_NOT_USE
#        define bms_getData bms_getData_INTERNAL_API_DO_NOT_USE
#        define bms_open bms_open_INTERNAL_API_DO_NOT_USE
#        define bms_search bms_search_INTERNAL_API_DO_NOT_USE
#        define bms_setTargetString bms_setTargetString_INTERNAL_API_DO_NOT_USE
#        define pl_addFontRun pl_addFontRun_INTERNAL_API_DO_NOT_USE
#        define pl_addLocaleRun pl_addLocaleRun_INTERNAL_API_DO_NOT_USE
#        define pl_addValueRun pl_addValueRun_INTERNAL_API_DO_NOT_USE
#        define pl_close pl_close_INTERNAL_API_DO_NOT_USE
#        define pl_closeFontRuns pl_closeFontRuns_INTERNAL_API_DO_NOT_USE
#        define pl_closeLine pl_closeLine_INTERNAL_API_DO_NOT_USE
#        define pl_closeLocaleRuns pl_closeLocaleRuns_INTERNAL_API_DO_NOT_USE
#        define pl_closeValueRuns pl_closeValueRuns_INTERNAL_API_DO_NOT_USE
#        define pl_countLineRuns pl_countLineRuns_INTERNAL_API_DO_NOT_USE
#        define pl_getAscent pl_getAscent_INTERNAL_API_DO_NOT_USE
#        define pl_getDescent pl_getDescent_INTERNAL_API_DO_NOT_USE
#        define pl_getFontRunCount pl_getFontRunCount_INTERNAL_API_DO_NOT_USE
#        define pl_getFontRunFont pl_getFontRunFont_INTERNAL_API_DO_NOT_USE
#        define pl_getFontRunLastLimit pl_getFontRunLastLimit_INTERNAL_API_DO_NOT_USE
#        define pl_getFontRunLimit pl_getFontRunLimit_INTERNAL_API_DO_NOT_USE
#        define pl_getLeading pl_getLeading_INTERNAL_API_DO_NOT_USE
#        define pl_getLineAscent pl_getLineAscent_INTERNAL_API_DO_NOT_USE
#        define pl_getLineDescent pl_getLineDescent_INTERNAL_API_DO_NOT_USE
#        define pl_getLineLeading pl_getLineLeading_INTERNAL_API_DO_NOT_USE
#        define pl_getLineVisualRun pl_getLineVisualRun_INTERNAL_API_DO_NOT_USE
#        define pl_getLineWidth pl_getLineWidth_INTERNAL_API_DO_NOT_USE
#        define pl_getLocaleRunCount pl_getLocaleRunCount_INTERNAL_API_DO_NOT_USE
#        define pl_getLocaleRunLastLimit pl_getLocaleRunLastLimit_INTERNAL_API_DO_NOT_USE
#        define pl_getLocaleRunLimit pl_getLocaleRunLimit_INTERNAL_API_DO_NOT_USE
#        define pl_getLocaleRunLocale pl_getLocaleRunLocale_INTERNAL_API_DO_NOT_USE
#        define pl_getParagraphLevel pl_getParagraphLevel_INTERNAL_API_DO_NOT_USE
#        define pl_getTextDirection pl_getTextDirection_INTERNAL_API_DO_NOT_USE
#        define pl_getValueRunCount pl_getValueRunCount_INTERNAL_API_DO_NOT_USE
#        define pl_getValueRunLastLimit pl_getValueRunLastLimit_INTERNAL_API_DO_NOT_USE
#        define pl_getValueRunLimit pl_getValueRunLimit_INTERNAL_API_DO_NOT_USE
#        define pl_getValueRunValue pl_getValueRunValue_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunAscent pl_getVisualRunAscent_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunDescent pl_getVisualRunDescent_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunDirection pl_getVisualRunDirection_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunFont pl_getVisualRunFont_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunGlyphCount pl_getVisualRunGlyphCount_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunGlyphToCharMap pl_getVisualRunGlyphToCharMap_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunGlyphs pl_getVisualRunGlyphs_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunLeading pl_getVisualRunLeading_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunPositions pl_getVisualRunPositions_INTERNAL_API_DO_NOT_USE
#        define pl_line pl_line_INTERNAL_API_DO_NOT_USE
#        define pl_nextLine pl_nextLine_INTERNAL_API_DO_NOT_USE
#        define pl_openEmptyFontRuns pl_openEmptyFontRuns_INTERNAL_API_DO_NOT_USE
#        define pl_openEmptyLocaleRuns pl_openEmptyLocaleRuns_INTERNAL_API_DO_NOT_USE
#        define pl_openEmptyValueRuns pl_openEmptyValueRuns_INTERNAL_API_DO_NOT_USE
#        define pl_openFontRuns pl_openFontRuns_INTERNAL_API_DO_NOT_USE
#        define pl_openLocaleRuns pl_openLocaleRuns_INTERNAL_API_DO_NOT_USE
#        define pl_openValueRuns pl_openValueRuns_INTERNAL_API_DO_NOT_USE
#        define pl_paragraph pl_paragraph_INTERNAL_API_DO_NOT_USE
#        define pl_reflow pl_reflow_INTERNAL_API_DO_NOT_USE
#        define pl_resetFontRuns pl_resetFontRuns_INTERNAL_API_DO_NOT_USE
#        define pl_resetLocaleRuns pl_resetLocaleRuns_INTERNAL_API_DO_NOT_USE
#        define pl_resetValueRuns pl_resetValueRuns_INTERNAL_API_DO_NOT_USE
#        define pl_visualRun pl_visualRun_INTERNAL_API_DO_NOT_USE
#        define ucd_close ucd_close_INTERNAL_API_DO_NOT_USE
#        define ucd_flushCache ucd_flushCache_INTERNAL_API_DO_NOT_USE
#        define ucd_freeCache ucd_freeCache_INTERNAL_API_DO_NOT_USE
#        define ucd_getCollator ucd_getCollator_INTERNAL_API_DO_NOT_USE
#        define ucd_open ucd_open_INTERNAL_API_DO_NOT_USE
#        define ucol_equals ucol_equals_INTERNAL_API_DO_NOT_USE
#        define ucol_forceHanImplicit ucol_forceHanImplicit_INTERNAL_API_DO_NOT_USE
#        define ucol_forgetUCA ucol_forgetUCA_INTERNAL_API_DO_NOT_USE
#        define ucol_getAttributeOrDefault ucol_getAttributeOrDefault_INTERNAL_API_DO_NOT_USE
#        define ucol_getReorderCodes ucol_getReorderCodes_INTERNAL_API_DO_NOT_USE
#        define ucol_getUnsafeSet ucol_getUnsafeSet_INTERNAL_API_DO_NOT_USE
#        define ucol_nextProcessed ucol_nextProcessed_INTERNAL_API_DO_NOT_USE
#        define ucol_prepareShortStringOpen ucol_prepareShortStringOpen_INTERNAL_API_DO_NOT_USE
#        define ucol_previousProcessed ucol_previousProcessed_INTERNAL_API_DO_NOT_USE
#        define ucol_setReorderCodes ucol_setReorderCodes_INTERNAL_API_DO_NOT_USE
#        define udat_applyPatternRelative udat_applyPatternRelative_INTERNAL_API_DO_NOT_USE
#        define udat_toPatternRelativeDate udat_toPatternRelativeDate_INTERNAL_API_DO_NOT_USE
#        define udat_toPatternRelativeTime udat_toPatternRelativeTime_INTERNAL_API_DO_NOT_USE
#        define uplug_getConfiguration uplug_getConfiguration_INTERNAL_API_DO_NOT_USE
#        define uplug_getContext uplug_getContext_INTERNAL_API_DO_NOT_USE
#        define uplug_getCurrentLevel uplug_getCurrentLevel_INTERNAL_API_DO_NOT_USE
#        define uplug_getLibrary uplug_getLibrary_INTERNAL_API_DO_NOT_USE
#        define uplug_getLibraryName uplug_getLibraryName_INTERNAL_API_DO_NOT_USE
#        define uplug_getPlugLevel uplug_getPlugLevel_INTERNAL_API_DO_NOT_USE
#        define uplug_getPlugLoadStatus uplug_getPlugLoadStatus_INTERNAL_API_DO_NOT_USE
#        define uplug_getPlugName uplug_getPlugName_INTERNAL_API_DO_NOT_USE
#        define uplug_getSymbolName uplug_getSymbolName_INTERNAL_API_DO_NOT_USE
#        define uplug_loadPlugFromEntrypoint uplug_loadPlugFromEntrypoint_INTERNAL_API_DO_NOT_USE
#        define uplug_loadPlugFromLibrary uplug_loadPlugFromLibrary_INTERNAL_API_DO_NOT_USE
#        define uplug_nextPlug uplug_nextPlug_INTERNAL_API_DO_NOT_USE
#        define uplug_removePlug uplug_removePlug_INTERNAL_API_DO_NOT_USE
#        define uplug_setContext uplug_setContext_INTERNAL_API_DO_NOT_USE
#        define uplug_setPlugLevel uplug_setPlugLevel_INTERNAL_API_DO_NOT_USE
#        define uplug_setPlugName uplug_setPlugName_INTERNAL_API_DO_NOT_USE
#        define uplug_setPlugNoUnload uplug_setPlugNoUnload_INTERNAL_API_DO_NOT_USE
#        define uprv_getDefaultCodepage uprv_getDefaultCodepage_INTERNAL_API_DO_NOT_USE
#        define uprv_getDefaultLocaleID uprv_getDefaultLocaleID_INTERNAL_API_DO_NOT_USE
#        define ures_openFillIn ures_openFillIn_INTERNAL_API_DO_NOT_USE
#        define usearch_search usearch_search_INTERNAL_API_DO_NOT_USE
#        define usearch_searchBackwards usearch_searchBackwards_INTERNAL_API_DO_NOT_USE
#        define utext_caseCompare utext_caseCompare_INTERNAL_API_DO_NOT_USE
#        define utext_caseCompareNativeLimit utext_caseCompareNativeLimit_INTERNAL_API_DO_NOT_USE
#        define utext_compare utext_compare_INTERNAL_API_DO_NOT_USE
#        define utext_compareNativeLimit utext_compareNativeLimit_INTERNAL_API_DO_NOT_USE
#        define utf8_appendCharSafeBody utf8_appendCharSafeBody_INTERNAL_API_DO_NOT_USE
#        define utf8_back1SafeBody utf8_back1SafeBody_INTERNAL_API_DO_NOT_USE
#        define utf8_countTrailBytes utf8_countTrailBytes_INTERNAL_API_DO_NOT_USE
#        define utf8_nextCharSafeBody utf8_nextCharSafeBody_INTERNAL_API_DO_NOT_USE
#        define utf8_prevCharSafeBody utf8_prevCharSafeBody_INTERNAL_API_DO_NOT_USE
#    else
#        define RegexPatternDump_4_6 RegexPatternDump_INTERNAL_API_DO_NOT_USE
#        define bms_close_4_6 bms_close_INTERNAL_API_DO_NOT_USE
#        define bms_empty_4_6 bms_empty_INTERNAL_API_DO_NOT_USE
#        define bms_getData_4_6 bms_getData_INTERNAL_API_DO_NOT_USE
#        define bms_open_4_6 bms_open_INTERNAL_API_DO_NOT_USE
#        define bms_search_4_6 bms_search_INTERNAL_API_DO_NOT_USE
#        define bms_setTargetString_4_6 bms_setTargetString_INTERNAL_API_DO_NOT_USE
#        define pl_addFontRun_4_6 pl_addFontRun_INTERNAL_API_DO_NOT_USE
#        define pl_addLocaleRun_4_6 pl_addLocaleRun_INTERNAL_API_DO_NOT_USE
#        define pl_addValueRun_4_6 pl_addValueRun_INTERNAL_API_DO_NOT_USE
#        define pl_close_4_6 pl_close_INTERNAL_API_DO_NOT_USE
#        define pl_closeFontRuns_4_6 pl_closeFontRuns_INTERNAL_API_DO_NOT_USE
#        define pl_closeLine_4_6 pl_closeLine_INTERNAL_API_DO_NOT_USE
#        define pl_closeLocaleRuns_4_6 pl_closeLocaleRuns_INTERNAL_API_DO_NOT_USE
#        define pl_closeValueRuns_4_6 pl_closeValueRuns_INTERNAL_API_DO_NOT_USE
#        define pl_countLineRuns_4_6 pl_countLineRuns_INTERNAL_API_DO_NOT_USE
#        define pl_getAscent_4_6 pl_getAscent_INTERNAL_API_DO_NOT_USE
#        define pl_getDescent_4_6 pl_getDescent_INTERNAL_API_DO_NOT_USE
#        define pl_getFontRunCount_4_6 pl_getFontRunCount_INTERNAL_API_DO_NOT_USE
#        define pl_getFontRunFont_4_6 pl_getFontRunFont_INTERNAL_API_DO_NOT_USE
#        define pl_getFontRunLastLimit_4_6 pl_getFontRunLastLimit_INTERNAL_API_DO_NOT_USE
#        define pl_getFontRunLimit_4_6 pl_getFontRunLimit_INTERNAL_API_DO_NOT_USE
#        define pl_getLeading_4_6 pl_getLeading_INTERNAL_API_DO_NOT_USE
#        define pl_getLineAscent_4_6 pl_getLineAscent_INTERNAL_API_DO_NOT_USE
#        define pl_getLineDescent_4_6 pl_getLineDescent_INTERNAL_API_DO_NOT_USE
#        define pl_getLineLeading_4_6 pl_getLineLeading_INTERNAL_API_DO_NOT_USE
#        define pl_getLineVisualRun_4_6 pl_getLineVisualRun_INTERNAL_API_DO_NOT_USE
#        define pl_getLineWidth_4_6 pl_getLineWidth_INTERNAL_API_DO_NOT_USE
#        define pl_getLocaleRunCount_4_6 pl_getLocaleRunCount_INTERNAL_API_DO_NOT_USE
#        define pl_getLocaleRunLastLimit_4_6 pl_getLocaleRunLastLimit_INTERNAL_API_DO_NOT_USE
#        define pl_getLocaleRunLimit_4_6 pl_getLocaleRunLimit_INTERNAL_API_DO_NOT_USE
#        define pl_getLocaleRunLocale_4_6 pl_getLocaleRunLocale_INTERNAL_API_DO_NOT_USE
#        define pl_getParagraphLevel_4_6 pl_getParagraphLevel_INTERNAL_API_DO_NOT_USE
#        define pl_getTextDirection_4_6 pl_getTextDirection_INTERNAL_API_DO_NOT_USE
#        define pl_getValueRunCount_4_6 pl_getValueRunCount_INTERNAL_API_DO_NOT_USE
#        define pl_getValueRunLastLimit_4_6 pl_getValueRunLastLimit_INTERNAL_API_DO_NOT_USE
#        define pl_getValueRunLimit_4_6 pl_getValueRunLimit_INTERNAL_API_DO_NOT_USE
#        define pl_getValueRunValue_4_6 pl_getValueRunValue_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunAscent_4_6 pl_getVisualRunAscent_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunDescent_4_6 pl_getVisualRunDescent_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunDirection_4_6 pl_getVisualRunDirection_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunFont_4_6 pl_getVisualRunFont_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunGlyphCount_4_6 pl_getVisualRunGlyphCount_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunGlyphToCharMap_4_6 pl_getVisualRunGlyphToCharMap_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunGlyphs_4_6 pl_getVisualRunGlyphs_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunLeading_4_6 pl_getVisualRunLeading_INTERNAL_API_DO_NOT_USE
#        define pl_getVisualRunPositions_4_6 pl_getVisualRunPositions_INTERNAL_API_DO_NOT_USE
#        define pl_line_4_6 pl_line_INTERNAL_API_DO_NOT_USE
#        define pl_nextLine_4_6 pl_nextLine_INTERNAL_API_DO_NOT_USE
#        define pl_openEmptyFontRuns_4_6 pl_openEmptyFontRuns_INTERNAL_API_DO_NOT_USE
#        define pl_openEmptyLocaleRuns_4_6 pl_openEmptyLocaleRuns_INTERNAL_API_DO_NOT_USE
#        define pl_openEmptyValueRuns_4_6 pl_openEmptyValueRuns_INTERNAL_API_DO_NOT_USE
#        define pl_openFontRuns_4_6 pl_openFontRuns_INTERNAL_API_DO_NOT_USE
#        define pl_openLocaleRuns_4_6 pl_openLocaleRuns_INTERNAL_API_DO_NOT_USE
#        define pl_openValueRuns_4_6 pl_openValueRuns_INTERNAL_API_DO_NOT_USE
#        define pl_paragraph_4_6 pl_paragraph_INTERNAL_API_DO_NOT_USE
#        define pl_reflow_4_6 pl_reflow_INTERNAL_API_DO_NOT_USE
#        define pl_resetFontRuns_4_6 pl_resetFontRuns_INTERNAL_API_DO_NOT_USE
#        define pl_resetLocaleRuns_4_6 pl_resetLocaleRuns_INTERNAL_API_DO_NOT_USE
#        define pl_resetValueRuns_4_6 pl_resetValueRuns_INTERNAL_API_DO_NOT_USE
#        define pl_visualRun_4_6 pl_visualRun_INTERNAL_API_DO_NOT_USE
#        define ucd_close_4_6 ucd_close_INTERNAL_API_DO_NOT_USE
#        define ucd_flushCache_4_6 ucd_flushCache_INTERNAL_API_DO_NOT_USE
#        define ucd_freeCache_4_6 ucd_freeCache_INTERNAL_API_DO_NOT_USE
#        define ucd_getCollator_4_6 ucd_getCollator_INTERNAL_API_DO_NOT_USE
#        define ucd_open_4_6 ucd_open_INTERNAL_API_DO_NOT_USE
#        define ucol_equals_4_6 ucol_equals_INTERNAL_API_DO_NOT_USE
#        define ucol_forceHanImplicit_4_6 ucol_forceHanImplicit_INTERNAL_API_DO_NOT_USE
#        define ucol_forgetUCA_4_6 ucol_forgetUCA_INTERNAL_API_DO_NOT_USE
#        define ucol_getAttributeOrDefault_4_6 ucol_getAttributeOrDefault_INTERNAL_API_DO_NOT_USE
#        define ucol_getReorderCodes_4_6 ucol_getReorderCodes_INTERNAL_API_DO_NOT_USE
#        define ucol_getUnsafeSet_4_6 ucol_getUnsafeSet_INTERNAL_API_DO_NOT_USE
#        define ucol_nextProcessed_4_6 ucol_nextProcessed_INTERNAL_API_DO_NOT_USE
#        define ucol_prepareShortStringOpen_4_6 ucol_prepareShortStringOpen_INTERNAL_API_DO_NOT_USE
#        define ucol_previousProcessed_4_6 ucol_previousProcessed_INTERNAL_API_DO_NOT_USE
#        define ucol_setReorderCodes_4_6 ucol_setReorderCodes_INTERNAL_API_DO_NOT_USE
#        define udat_applyPatternRelative_4_6 udat_applyPatternRelative_INTERNAL_API_DO_NOT_USE
#        define udat_toPatternRelativeDate_4_6 udat_toPatternRelativeDate_INTERNAL_API_DO_NOT_USE
#        define udat_toPatternRelativeTime_4_6 udat_toPatternRelativeTime_INTERNAL_API_DO_NOT_USE
#        define uplug_getConfiguration_4_6 uplug_getConfiguration_INTERNAL_API_DO_NOT_USE
#        define uplug_getContext_4_6 uplug_getContext_INTERNAL_API_DO_NOT_USE
#        define uplug_getCurrentLevel_4_6 uplug_getCurrentLevel_INTERNAL_API_DO_NOT_USE
#        define uplug_getLibrary_4_6 uplug_getLibrary_INTERNAL_API_DO_NOT_USE
#        define uplug_getLibraryName_4_6 uplug_getLibraryName_INTERNAL_API_DO_NOT_USE
#        define uplug_getPlugLevel_4_6 uplug_getPlugLevel_INTERNAL_API_DO_NOT_USE
#        define uplug_getPlugLoadStatus_4_6 uplug_getPlugLoadStatus_INTERNAL_API_DO_NOT_USE
#        define uplug_getPlugName_4_6 uplug_getPlugName_INTERNAL_API_DO_NOT_USE
#        define uplug_getSymbolName_4_6 uplug_getSymbolName_INTERNAL_API_DO_NOT_USE
#        define uplug_loadPlugFromEntrypoint_4_6 uplug_loadPlugFromEntrypoint_INTERNAL_API_DO_NOT_USE
#        define uplug_loadPlugFromLibrary_4_6 uplug_loadPlugFromLibrary_INTERNAL_API_DO_NOT_USE
#        define uplug_nextPlug_4_6 uplug_nextPlug_INTERNAL_API_DO_NOT_USE
#        define uplug_removePlug_4_6 uplug_removePlug_INTERNAL_API_DO_NOT_USE
#        define uplug_setContext_4_6 uplug_setContext_INTERNAL_API_DO_NOT_USE
#        define uplug_setPlugLevel_4_6 uplug_setPlugLevel_INTERNAL_API_DO_NOT_USE
#        define uplug_setPlugName_4_6 uplug_setPlugName_INTERNAL_API_DO_NOT_USE
#        define uplug_setPlugNoUnload_4_6 uplug_setPlugNoUnload_INTERNAL_API_DO_NOT_USE
#        define uprv_getDefaultCodepage_4_6 uprv_getDefaultCodepage_INTERNAL_API_DO_NOT_USE
#        define uprv_getDefaultLocaleID_4_6 uprv_getDefaultLocaleID_INTERNAL_API_DO_NOT_USE
#        define ures_openFillIn_4_6 ures_openFillIn_INTERNAL_API_DO_NOT_USE
#        define usearch_search_4_6 usearch_search_INTERNAL_API_DO_NOT_USE
#        define usearch_searchBackwards_4_6 usearch_searchBackwards_INTERNAL_API_DO_NOT_USE
#        define utext_caseCompareNativeLimit_4_6 utext_caseCompareNativeLimit_INTERNAL_API_DO_NOT_USE
#        define utext_caseCompare_4_6 utext_caseCompare_INTERNAL_API_DO_NOT_USE
#        define utext_compareNativeLimit_4_6 utext_compareNativeLimit_INTERNAL_API_DO_NOT_USE
#        define utext_compare_4_6 utext_compare_INTERNAL_API_DO_NOT_USE
#        define utf8_appendCharSafeBody_4_6 utf8_appendCharSafeBody_INTERNAL_API_DO_NOT_USE
#        define utf8_back1SafeBody_4_6 utf8_back1SafeBody_INTERNAL_API_DO_NOT_USE
#        define utf8_countTrailBytes_4_6 utf8_countTrailBytes_INTERNAL_API_DO_NOT_USE
#        define utf8_nextCharSafeBody_4_6 utf8_nextCharSafeBody_INTERNAL_API_DO_NOT_USE
#        define utf8_prevCharSafeBody_4_6 utf8_prevCharSafeBody_INTERNAL_API_DO_NOT_USE
#    endif /* U_DISABLE_RENAMING */

#endif /* U_HIDE_INTERNAL_API */
#endif /* UINTRNAL_H */

