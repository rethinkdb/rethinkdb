/******************************************************************************
*
*   Copyright (C) 2001, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  stubdata.c
*
*   Define initialized data that will build into a valid, but empty
*   ICU data library.  Used to bootstrap the ICU build, which has these
*   dependencies:
*       ICU Common library depends on ICU data
*       ICU data requires data building tools.
*       ICU data building tools require the ICU common library.
*
*   The stub data library (for which this file is the source) is sufficient
*   for running the data building tools.
*
*/
#include "unicode/utypes.h"
#include "unicode/udata.h"
#include "unicode/uversion.h"
#include "ucmndata.h"


U_EXPORT const ICU_Data_Header U_ICUDATA_ENTRY_POINT = {
    {            /* DataHeader */
        {            /* MappedData */
            32,          /* headerSize */
            0xda,        /* magic1,  (see struct MappedData in udata.c)  */
            0x27,        /* magic2     */
        },
        {            /*UDataInfo   */
            sizeof(UDataInfo),      /* size        */
            0,                      /* reserved    */

#if U_IS_BIG_ENDIAN
            1,
#else
            0,
#endif

            U_CHARSET_FAMILY,
            sizeof(UChar),   
            0,               /* reserved      */
            {                /* data format identifier */
               0x54, 0x6f, 0x43, 0x50}, /* "ToCP" */
               {1, 0, 0, 0},   /* format version major, minor, milli, micro */
               {0, 0, 0, 0}    /* dataVersion   */
        },
    },
    {0,0,0,0,0,0,0,0},  /* Padding[8]   */ 
    0,                  /* count        */
    0,                  /* Reserved     */
    {                   /*  TOC structure */
/*        {    */
          0 , 0 , 0, 0  /* name and data entries.  Count says there are none,  */
                        /*  but put one in just in case.                       */
/*        }  */
    }
};


