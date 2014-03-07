/*
*******************************************************************************
*
*   Copyright (C) 1998-1999, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*
* File error.h
*
* Modification History:
*
*   Date        Name        Description
*   05/28/99    stephen     Creation.
*******************************************************************************
*/

#ifndef ERROR_H
#define ERROR_H 1

extern const char *gCurrentFileName;

void error   (uint32_t linenumber, const char *msg, ...);
void warning (uint32_t linenumber, const char *msg, ...);


/* Show warnings? */
U_CFUNC void setShowWarning(UBool val);
U_CFUNC UBool getShowWarning(void);

/* strict */
U_CFUNC void setStrict(UBool val);
U_CFUNC UBool isStrict(void);

/* verbosity */
U_CFUNC void setVerbose(UBool val);
U_CFUNC UBool isVerbose(void);
#endif
