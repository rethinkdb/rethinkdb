/*
**********************************************************************
* Copyright (c) 2002-2005, International Business Machines
* Corporation and others.  All Rights Reserved.
**********************************************************************
* 2005Nov22 Raymond Yang
* 
* migrate old test created by aliu to perf test framework.
*/

#include <stdio.h>

#include "unicode/utypes.h"
#include "unicode/uniset.h"
#include "unicode/uchar.h"
#include "unicode/usetiter.h"
#include "bitset.h"
#include "unicode/uperf.h"

static const char* PAT[] = {
    "['A-Za-z\\u00C0-\\u00C5\\u00C7-\\u00CF\\u00D1-\\u00D6\\u00D9-\\u00DD\\u00E0-\\u00E5\\u00E7-\\u00EF\\u00F1-\\u00F6\\u00F9-\\u00FD\\u00FF-\\u010F\\u0112-\\u0125\\u0128-\\u0130\\u0134-\\u0137\\u0139-\\u013E\\u0143-\\u0148\\u014C-\\u0151\\u0154-\\u0165\\u0168-\\u017E\\u01A0-\\u01A1\\u01AF-\\u01B0\\u01CD-\\u01DC\\u01DE-\\u01E1\\u01E6-\\u01ED\\u01F0\\u01F4-\\u01F5\\u01F8-\\u01FB\\u0200-\\u021B\\u021E-\\u021F\\u0226-\\u0233\\u1E00-\\u1E99\\u1EA0-\\u1EF9\\u212A-\\u212B]",

    "['.0-9A-Za-z~\\u00C0-\\u00C5\\u00C7-\\u00CF\\u00D1-\\u00D6\\u00D9-\\u00DD\\u00E0-\\u00E5\\u00E7-\\u00EF\\u00F1-\\u00F6\\u00F9-\\u00FD\\u00FF-\\u010F\\u0112-\\u0125\\u0128-\\u0130\\u0134-\\u0137\\u0139-\\u013E\\u0143-\\u0148\\u014C-\\u0151\\u0154-\\u0165\\u0168-\\u017E\\u01A0-\\u01A1\\u01AF-\\u01B0\\u01CD-\\u01DC\\u01DE-\\u01E3\\u01E6-\\u01ED\\u01F0\\u01F4-\\u01F5\\u01F8-\\u021B\\u021E-\\u021F\\u0226-\\u0233\\u0301\\u0303-\\u0304\\u0306-\\u0307\\u0310\\u0314-\\u0315\\u0323\\u0325\\u0331\\u0341\\u0344\\u0385-\\u0386\\u0388-\\u038A\\u038C\\u038E-\\u0390\\u03AC-\\u03B0\\u03CC-\\u03CE\\u03D3\\u0403\\u040C\\u040E\\u0419\\u0439\\u0453\\u045C\\u045E\\u04C1-\\u04C2\\u04D0-\\u04D1\\u04D6-\\u04D7\\u04E2-\\u04E3\\u04EE-\\u04EF\\u1E00-\\u1E99\\u1EA0-\\u1EF9\\u1F01\\u1F03-\\u1F05\\u1F07\\u1F09\\u1F0B-\\u1F0D\\u1F0F\\u1F11\\u1F13-\\u1F15\\u1F19\\u1F1B-\\u1F1D\\u1F21\\u1F23-\\u1F25\\u1F27\\u1F29\\u1F2B-\\u1F2D\\u1F2F\\u1F31\\u1F33-\\u1F35\\u1F37\\u1F39\\u1F3B-\\u1F3D\\u1F3F\\u1F41\\u1F43-\\u1F45\\u1F49\\u1F4B-\\u1F4D\\u1F51\\u1F53-\\u1F55\\u1F57\\u1F59\\u1F5B\\u1F5D\\u1F5F\\u1F61\\u1F63-\\u1F65\\u1F67\\u1F69\\u1F6B-\\u1F6D\\u1F6F\\u1F71\\u1F73\\u1F75\\u1F77\\u1F79\\u1F7B\\u1F7D\\u1F81\\u1F83-\\u1F85\\u1F87\\u1F89\\u1F8B-\\u1F8D\\u1F8F\\u1F91\\u1F93-\\u1F95\\u1F97\\u1F99\\u1F9B-\\u1F9D\\u1F9F\\u1FA1\\u1FA3-\\u1FA5\\u1FA7\\u1FA9\\u1FAB-\\u1FAD\\u1FAF-\\u1FB1\\u1FB4\\u1FB8-\\u1FB9\\u1FBB\\u1FC4\\u1FC9\\u1FCB\\u1FCE\\u1FD0-\\u1FD1\\u1FD3\\u1FD8-\\u1FD9\\u1FDB\\u1FDE\\u1FE0-\\u1FE1\\u1FE3\\u1FE5\\u1FE8-\\u1FE9\\u1FEB-\\u1FEC\\u1FEE\\u1FF4\\u1FF9\\u1FFB\\u212A-\\u212B\\uE04D\\uE064]",

    "[\\u0901-\\u0903\\u0905-\\u0939\\u093C-\\u094D\\u0950-\\u0954\\u0958-\\u096F]",
};

class CmdPattern : public UPerfFunction {
private:
    UnicodeString pat;
    UnicodeSet set;
public:
    CmdPattern(const char * pattern):pat(pattern,""){
    }
    virtual long getOperationsPerIteration(){
        return 1;
    }
    virtual void call(UErrorCode* pErrorCode){
        set.applyPattern(pat, *pErrorCode);
    }
};

class CmdOp : public UPerfFunction {
private:
    UnicodeSet us;
    BitSet bs;
    int32_t total;
    void (CmdOp::*op) ();
public:
    CmdOp(UCharCategory prop, void (CmdOp::*op)()):op(op){
        total = 0;
        bs.clearAll();
        for (UChar32 cp=0; cp<0x110000; ++cp) {
            if (u_charType(cp) == prop) {
                bs.set((int32_t) cp);
                ++total;
            }
        }
    }
    virtual long getOperationsPerIteration(){
        return total;
    }

    virtual void call(UErrorCode* pErrorCode){
        (this->*op)();
    }
    void add (void){
        us.clear();
        for (UChar32 cp=0; cp<0x110000; ++cp) {
            if (bs.get((int32_t) cp)) {
                us.add(cp);
            }
        }
    }

    void contains(void){
        int32_t temp = 0;
        us.clear();
        for (UChar32 cp=0; cp<0x110000; ++cp) {
            if (us.contains(cp)) {
                temp += cp;
            }
        }
    }

    void iterator(void){
        int32_t temp = 0;
        UnicodeSetIterator uit(us);
        while (uit.next()) {
            temp += uit.getCodepoint();
        }
    }
};

class  UsetPerformanceTest : public UPerfTest{
public:
    UsetPerformanceTest(int32_t argc, const char *argv[], UErrorCode &status) :UPerfTest(argc,argv,status){
    }

    virtual UPerfFunction* runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL ){
        switch (index) {
            case 0: name = "titlecase_letter_add"; 
                if (exec) return new CmdOp(U_TITLECASE_LETTER, &CmdOp::add) ; break;
            case 1: name = "titlecase_letter_contains"; 
                if (exec) return new CmdOp(U_TITLECASE_LETTER, &CmdOp::contains)  ; break;
            case 2: name = "titlecase_letter_iterator"; 
                if (exec) return new CmdOp(U_TITLECASE_LETTER, &CmdOp::iterator)  ; break;
            case 3: name = "unassigned_add"; 
                if (exec) return new CmdOp(U_UNASSIGNED, &CmdOp::add)  ; break;
            case 4: name = "unassigned_contains"; 
                if (exec) return new CmdOp(U_UNASSIGNED, &CmdOp::contains)  ; break;
            case 5: name = "unassigned_iterator"; 
                if (exec) return new CmdOp(U_UNASSIGNED, &CmdOp::iterator)  ; break;
            case 6: name = "pattern1"; 
                if (exec) return new CmdPattern(PAT[0])  ; break;
            case 7: name = "pattern2";
                if (exec) return new CmdPattern(PAT[1])  ; break;
            case 8: name = "pattern3";
                if (exec) return new CmdPattern(PAT[2])  ; break;
            default: name = ""; break;
        }
        return NULL;
    }
};


int main(int argc, const char *argv[])
{
    UErrorCode status = U_ZERO_ERROR;
    UsetPerformanceTest test(argc, argv, status);

	if (U_FAILURE(status)){
        printf("The error is %s\n", u_errorName(status));
        return status;
    }
        
    if (test.run() == FALSE){
        fprintf(stderr, "FAILED: Tests could not be run please check the "
			            "arguments.\n");
        return -1;
    }
    return 0;
}
