/*
******************************************************************************
*
*   Copyright (C) 1998-2004, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*
* File uprintf.c
*
* Modification History:
*
*   Date        Name        Description
*   11/19/98    stephen     Creation.
*   03/12/99    stephen     Modified for new C API.
*                           Added conversion from default codepage.
*   08/07/2003  george      Reunify printf implementations
******************************************************************************
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/ustdio.h"
#include "unicode/ustring.h"
#include "unicode/unum.h"
#include "unicode/udat.h"
#include "unicode/putil.h"

#include "uprintf.h"
#include "ufile.h"
#include "locbund.h"

#include "cmemory.h"

static int32_t U_EXPORT2
u_printf_write(void          *context,
               const UChar   *str,
               int32_t       count)
{
    return u_file_write(str, count, (UFILE *)context);
}

static int32_t
u_printf_pad_and_justify(void                        *context,
                         const u_printf_spec_info    *info,
                         const UChar                 *result,
                         int32_t                     resultLen)
{
    UFILE   *output = (UFILE *)context;
    int32_t written, i;

    /* pad and justify, if needed */
    if(info->fWidth != -1 && resultLen < info->fWidth) {
        /* left justify */
        if(info->fLeft) {
            written = u_file_write(result, resultLen, output);
            for(i = 0; i < info->fWidth - resultLen; ++i) {
                written += u_file_write(&info->fPadChar, 1, output);
            }
        }
        /* right justify */
        else {
            written = 0;
            for(i = 0; i < info->fWidth - resultLen; ++i) {
                written += u_file_write(&info->fPadChar, 1, output);
            }
            written += u_file_write(result, resultLen, output);
        }
    }
    /* just write the formatted output */
    else {
        written = u_file_write(result, resultLen, output);
    }

    return written;
}

U_CAPI int32_t U_EXPORT2 
u_fprintf(    UFILE        *f,
          const char    *patternSpecification,
          ... )
{
    va_list ap;
    int32_t count;

    va_start(ap, patternSpecification);
    count = u_vfprintf(f, patternSpecification, ap);
    va_end(ap);

    return count;
}

U_CAPI int32_t U_EXPORT2 
u_fprintf_u(    UFILE        *f,
            const UChar    *patternSpecification,
            ... )
{
    va_list ap;
    int32_t count;

    va_start(ap, patternSpecification);
    count = u_vfprintf_u(f, patternSpecification, ap);
    va_end(ap);

    return count;
}

U_CAPI int32_t  U_EXPORT2 /* U_CAPI ... U_EXPORT2 added by Peter Kirk 17 Nov 2001 */
u_vfprintf(    UFILE        *f,
           const char    *patternSpecification,
           va_list        ap)
{
    int32_t count;
    UChar *pattern;
    UChar buffer[UFMT_DEFAULT_BUFFER_SIZE];
    int32_t size = (int32_t)strlen(patternSpecification) + 1;

    /* convert from the default codepage to Unicode */
    if (size >= MAX_UCHAR_BUFFER_SIZE(buffer)) {
        pattern = (UChar *)uprv_malloc(size * sizeof(UChar));
        if(pattern == 0) {
            return 0;
        }
    }
    else {
        pattern = buffer;
    }
    u_charsToUChars(patternSpecification, pattern, size);

    /* do the work */
    count = u_vfprintf_u(f, pattern, ap);

    /* clean up */
    if (pattern != buffer) {
        uprv_free(pattern);
    }

    return count;
}

static const u_printf_stream_handler g_stream_handler = {
    u_printf_write,
    u_printf_pad_and_justify
};

U_CAPI int32_t  U_EXPORT2 /* U_CAPI ... U_EXPORT2 added by Peter Kirk 17 Nov 2001 */
u_vfprintf_u(    UFILE        *f,
             const UChar    *patternSpecification,
             va_list        ap)
{
    int32_t          written = 0;   /* haven't written anything yet */

    /* parse and print the whole format string */
    u_printf_parse(&g_stream_handler, patternSpecification, f, NULL, &f->str.fBundle, &written, ap);

    /* return # of UChars written */
    return written;
}

#endif /* #if !UCONFIG_NO_FORMATTING */

