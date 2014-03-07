//
//********************************************************************
//   Copyright (C) 2002-2003, International Business Machines
//   Corporation and others.  All Rights Reserved.
//********************************************************************
//
// File converttest.cpp
//

#include "threadtest.h"
#include "unicode/utypes.h"
#include "unicode/ucnv.h"
#include "unicode/uclean.h"
#include "stdio.h"

U_CAPI UBool U_EXPORT2 ucnv_cleanup();

class ConvertThreadTest: public AbstractThreadTest {
public:
                    ConvertThreadTest();
    virtual        ~ConvertThreadTest();
    virtual void    check();
    virtual void    runOnce();

private:
    UConverter      *fCnv;
};


ConvertThreadTest::ConvertThreadTest() {
    UErrorCode    err = U_ZERO_ERROR;

    fCnv = ucnv_open("gb18030", &err);
    if (U_FAILURE(err)) {
        fprintf(stderr, "ConvertTest - could not ucnv_open(\"gb18030\")\n");
        fCnv = NULL;
    }
};


ConvertThreadTest::~ConvertThreadTest() {
    ucnv_close(fCnv);
    fCnv = 0;
}

void ConvertThreadTest::runOnce() {
    UErrorCode     err = U_ZERO_ERROR;
    UConverter     *cnv1;
    UConverter     *cnv2;
    char           buf[U_CNV_SAFECLONE_BUFFERSIZE];
    int32_t        bufSize = U_CNV_SAFECLONE_BUFFERSIZE;

    cnv1 = ucnv_open("shift_jis", &err);
    if (U_FAILURE(err)) {
        fprintf(stderr, "ucnv_open(\"shift_jis\") failed.\n");
    }

    cnv2 = ucnv_safeClone(fCnv,       // The source converter, common to all threads.
                          buf,  
                          &bufSize,  
                          &err);
    if (U_FAILURE(err)) {
        fprintf(stderr, "ucnv_safeClone() failed.\n");
    }
    ucnv_close(cnv1);
    ucnv_close(cnv2);
    ucnv_flushCache();
}

void ConvertThreadTest::check() {
    UErrorCode     err = U_ZERO_ERROR;

    if (fCnv) {ucnv_close(fCnv);}
    //if (ucnv_cleanup () == FALSE) {
    //    fprintf(stderr, "ucnv_cleanup() failed - cache was not empty.\n");
    //}
    fCnv = ucnv_open("gb18030", &err);
    if (U_FAILURE(err)) {
        fprintf(stderr, "ConvertTest::check() - could not redo ucnv_open(\"gb18030\")\n");
        fCnv = NULL;
    }
}


AbstractThreadTest *createConvertTest() {
    return new ConvertThreadTest();
}

