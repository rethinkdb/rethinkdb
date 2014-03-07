/*
**********************************************************************
*   Copyright (C) 2007-2007, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*
* File tzdate.c
*
* Author:  Michael Ow
*
**********************************************************************
*/

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "unicode/utypes.h"
#include "unicode/ustring.h"
#include "unicode/uclean.h"

#include "unicode/ucnv.h"
#include "unicode/udat.h"
#include "unicode/ucal.h"

#include "putilimp.h"

#define SIZE 80
#define OFFSET_MONTH 1

int64_t getSystemCurrentTime(char* systime, int year, int month, int day, int hour, int minute, int useCurrentTime);
void getICUCurrentTime(char* icutime, double timeToCheck);
void printTime(char* systime, char* icutime);

int main(int argc, char** argv) {
    char systime[SIZE];
    char icutime[SIZE];
    int year, month, day, hour, minute;
    int sysyear;
    int useCurrentTime;
    int64_t systemtime;
    
    sysyear = year = month = day = 0;
    
    if (argc <= 6) {
        fprintf(stderr, "Not enough arguments\n");
        return -1;
    }

    year = atoi(argv[1]);
    month = atoi(argv[2]);
    day = atoi(argv[3]);
    hour = atoi(argv[4]);
    minute = atoi(argv[5]);
    useCurrentTime = atoi(argv[6]);
    
    /* format year for system time */
    sysyear = year - 1900;
    
    systemtime = getSystemCurrentTime(systime, sysyear, month, day, hour, minute, useCurrentTime);
    getICUCurrentTime(icutime, systemtime * U_MILLIS_PER_SECOND);

    /* print out the times if failed */
    if (strcmp(systime, icutime) != 0) {
        printf("Failed\n");
        printTime(systime, icutime);
    }

    return 0;
}

void getICUCurrentTime(char* icutime, double timeToCheck) {
    UDateFormat *fmt;
    const UChar *tz = 0;
    UChar *s = 0;
    UDateFormatStyle style = UDAT_RELATIVE;
    UErrorCode status = U_ZERO_ERROR;
    int32_t len = 0;
    int i;

    fmt = udat_open(style, style, 0, tz, -1,NULL,0, &status);

    len = udat_format(fmt, timeToCheck, 0, len, 0, &status);

    if (status == U_BUFFER_OVERFLOW_ERROR)
        status = U_ZERO_ERROR;

    s = (UChar*) malloc(sizeof(UChar) * (len+1));

    if(s == 0) 
        goto finish;

    udat_format(fmt, timeToCheck, s, len + 1, 0, &status);
    
    if (U_FAILURE(status)) 
        goto finish;

    /* +1 to NULL terminate */
    for(i = 0; i < len+1; i++) {
        icutime[i] = (char)s[i];
    }

finish:
    udat_close(fmt);
    free(s);
}

int64_t getSystemCurrentTime(char* systime, int year, int month, int day, int hour, int minute, int useCurrentTime) {
    time_t now;
    struct tm ts;

    if (useCurrentTime){
        time(&now);
        ts = *localtime(&now);
    }
    else {
        memset(&ts, 0, sizeof(ts));
        ts.tm_year = year;
        ts.tm_mon = month - OFFSET_MONTH;
        ts.tm_mday = day;
        ts.tm_hour = hour;
        ts.tm_min = minute;

        now = mktime(&ts);
        ts = *localtime(&now);
    }

    /* Format the string */
    strftime(systime, sizeof(char) * 80, "%Y%m%d %I:%M %p", &ts);

    return (double)now;
}

void printTime(char* systime, char* icutime) {
    printf("System Time:  %s\n", systime);
    printf("ICU Time:     %s\n", icutime);
    printf("STD=%s DST=%s OFFSET=%d\n", uprv_tzname(0), uprv_tzname(1), uprv_timezone());
}

