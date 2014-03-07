/******************************************************************************
 *   Copyright (C) 2008-2009, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *******************************************************************************
 */

#ifndef __PKG_GENC_H__
#define __PKG_GENC_H__

#include "unicode/utypes.h"

U_INTERNAL void U_EXPORT2
printAssemblyHeadersToStdErr(void);

U_INTERNAL UBool U_EXPORT2
checkAssemblyHeaderName(const char* optAssembly);

U_INTERNAL void U_EXPORT2
writeCCode(const char *filename, const char *destdir, const char *optName, const char *optFilename, char *outFilePath);

U_INTERNAL void U_EXPORT2
writeAssemblyCode(const char *filename, const char *destdir, const char *optEntryPoint, const char *optFilename, char *outFilePath);

U_INTERNAL void U_EXPORT2
writeObjectCode(const char *filename, const char *destdir, const char *optEntryPoint, const char *optMatchArch, const char *optFilename, char *outFilePath);

#endif
