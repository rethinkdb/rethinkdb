/******************************************************************************
 *   Copyright (C) 2008-2009, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *******************************************************************************
 */

#ifndef __PKG_ICU_H__
#define __PKG_ICU_H__

#include "unicode/utypes.h"

#define U_PKG_RESERVED_CHARS "\"%&'()*+,-./:;<=>?_"

U_CAPI int U_EXPORT2
writePackageDatFile(const char *outFilename, const char *outComment,
                    const char *sourcePath, const char *addList, U_NAMESPACE_QUALIFIER Package *pkg,
                    char outType);

U_CAPI U_NAMESPACE_QUALIFIER Package * U_EXPORT2
readList(const char *filesPath, const char *listname, UBool readContents);

#endif
