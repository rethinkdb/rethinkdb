/*
*******************************************************************************
*
*   Copyright (C) 2009, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  errorcode.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2009mar10
*   created by: Markus W. Scherer
*/

#include "unicode/utypes.h"
#include "unicode/errorcode.h"

U_NAMESPACE_BEGIN

UErrorCode ErrorCode::reset() {
    UErrorCode code = errorCode;
    errorCode = U_ZERO_ERROR;
    return code;
}

void ErrorCode::assertSuccess() const {
    if(isFailure()) {
        handleFailure();
    }
}

const char* ErrorCode::errorName() const {
  return u_errorName(errorCode);
}

U_NAMESPACE_END
