/*
**********************************************************************
*   Copyright (C) 1999-2006 IBM Corp. All rights reserved.
**********************************************************************
*   Date        Name        Description
*   12/1/99    rgillam     Complete port from Java.
*   01/13/2000 helena      Added UErrorCode to ctors.
**********************************************************************
*/

#ifndef DBBI_H
#define DBBI_H

#include "unicode/rbbi.h"

#if !UCONFIG_NO_BREAK_ITERATION

/**
 * \file
 * \brief C++ API: Dictionary Based Break Iterator
 */
 
U_NAMESPACE_BEGIN

/**
 * An obsolete subclass of RuleBasedBreakIterator. Handling of dictionary-
 * based break iteration has been folded into the base class. This class
 * is deprecated as of ICU 3.6.
 */
 
#ifndef U_HIDE_DEPRECATED_API

typedef RuleBasedBreakIterator DictionaryBasedBreakIterator;

#endif

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_BREAK_ITERATION */

#endif
