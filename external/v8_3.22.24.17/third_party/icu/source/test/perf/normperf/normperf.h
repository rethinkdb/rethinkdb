/*
**********************************************************************
* Copyright (c) 2002-2006, International Business Machines
* Corporation and others.  All Rights Reserved.
**********************************************************************
**********************************************************************
*/
#ifndef _NORMPERF_H
#define _NORMPERF_H

#include "unicode/unorm.h"
#include "unicode/ustring.h"

#include "unicode/uperf.h"
#include <stdlib.h>

//  Stubs for Windows API functions when building on UNIXes.
//
#if defined(U_WINDOWS)
// do nothing
#else
#define _UNICODE
typedef int DWORD;
inline int FoldStringW(DWORD dwMapFlags, const UChar* lpSrcStr,int cchSrc, UChar* lpDestStr,int cchDest);
#endif

#define DEST_BUFFER_CAPACITY 6000
typedef int32_t (*NormFn)(const UChar* src,int32_t srcLen, UChar* dest,int32_t dstLen, int32_t options, UErrorCode* status);
typedef int32_t (*QuickCheckFn)(const UChar* src,int32_t srcLen, UNormalizationMode mode, int32_t options, UErrorCode* status);

class QuickCheckPerfFunction : public UPerfFunction{
private:
    ULine* lines;
    int32_t numLines;
    QuickCheckFn fn;
    UNormalizationMode mode;
    int32_t retVal;
    UBool uselen;
    const UChar* src;
    int32_t srcLen;
    UBool line_mode;
    int32_t options;

public:
    virtual void call(UErrorCode* status){
        if(line_mode==TRUE){
            if(uselen){
                for(int32_t i = 0; i< numLines; i++){
                    retVal =  (*fn)(lines[i].name,lines[i].len,mode, options, status);
                }
            }else{
                for(int32_t i = 0; i< numLines; i++){
                    retVal =  (*fn)(lines[i].name,-1,mode, options, status);
                }
            }
        }else{
            if(uselen){

                retVal =  (*fn)(src,srcLen,mode, options, status);
            }else{
                retVal =  (*fn)(src,-1,mode, options, status);
            }
        }

    }
    virtual long getOperationsPerIteration(){
        if(line_mode==TRUE){
            int32_t totalChars=0;
            for(int32_t i =0; i< numLines; i++){
                totalChars+= lines[i].len;
            }
            return totalChars;
        }else{
            return srcLen;
        }
    }
    QuickCheckPerfFunction(QuickCheckFn func, ULine* srcLines,int32_t srcNumLines, UNormalizationMode _mode, int32_t opts, UBool _uselen) : options(opts) {
        fn = func;
        lines = srcLines;
        numLines = srcNumLines;
        uselen = _uselen;
        mode = _mode;
        src = NULL;
        srcLen = 0;
        line_mode = TRUE;
    }
    QuickCheckPerfFunction(QuickCheckFn func, const UChar* source,int32_t sourceLen, UNormalizationMode _mode, int32_t opts, UBool _uselen) : options(opts) {
        fn = func;
        lines = NULL;
        numLines = 0;
        uselen = _uselen;
        mode = _mode;
        src = source;
        srcLen = sourceLen;
        line_mode = FALSE;
    }
};


class NormPerfFunction : public UPerfFunction{
private:
    ULine* lines;
    int32_t numLines;
    UChar dest[DEST_BUFFER_CAPACITY];
    UChar* pDest;
    int32_t destLen;
    NormFn fn;
    int32_t retVal;
    UBool uselen;
    const UChar* src;
    int32_t srcLen;
    UBool line_mode;
    int32_t options;

public:
    virtual void call(UErrorCode* status){
        if(line_mode==TRUE){
            if(uselen){
                for(int32_t i = 0; i< numLines; i++){
                    retVal =  (*fn)(lines[i].name,lines[i].len,pDest,destLen, options, status);
                }
            }else{
                for(int32_t i = 0; i< numLines; i++){
                    retVal =  (*fn)(lines[i].name,-1,pDest,destLen, options, status);
                }
            }
        }else{
            if(uselen){
                retVal =  (*fn)(src,srcLen,pDest,destLen, options, status);
            }else{
                retVal =  (*fn)(src,-1,pDest,destLen, options, status);
            }
        }
    }
    virtual long getOperationsPerIteration(){
        if(line_mode ==TRUE){
            int32_t totalChars=0;
            for(int32_t i =0; i< numLines; i++){
                totalChars+= lines[i].len;
            }
            return totalChars;
        }else{
            return srcLen;
        }
    }
    NormPerfFunction(NormFn func, int32_t opts, ULine* srcLines,int32_t srcNumLines,UBool _uselen) : options(opts) {
        fn = func;
        lines = srcLines;
        numLines = srcNumLines;
        uselen = _uselen;
        destLen = DEST_BUFFER_CAPACITY;
        pDest = dest;
        src = NULL;
        srcLen = 0;
        line_mode = TRUE;
    }
    NormPerfFunction(NormFn func, int32_t opts, const UChar* source,int32_t sourceLen,UBool _uselen) : options(opts) {
        fn = func;
        lines = NULL;
        numLines = 0;
        uselen = _uselen;
        destLen = sourceLen*3;
        pDest = (UChar*) malloc(destLen * U_SIZEOF_UCHAR);
        src = source;
        srcLen = sourceLen;
        line_mode = FALSE;
    }
    ~NormPerfFunction(){
        if(dest != pDest){
            free(pDest);
        }
    }
};



class  NormalizerPerformanceTest : public UPerfTest{
private:
    ULine* NFDFileLines;
    ULine* NFCFileLines;
    UChar* NFDBuffer;
    UChar* NFCBuffer;
    UChar* origBuffer;
    int32_t origBufferLen;
    int32_t NFDBufferLen;
    int32_t NFCBufferLen;
    int32_t options;

    void normalizeInput(ULine* dest,const UChar* src ,int32_t srcLen,UNormalizationMode mode, int32_t options);
    UChar* normalizeInput(int32_t& len, const UChar* src ,int32_t srcLen,UNormalizationMode mode, int32_t options);

public:

    NormalizerPerformanceTest(int32_t argc, const char* argv[], UErrorCode& status);
    ~NormalizerPerformanceTest();
    virtual UPerfFunction* runIndexedTest(int32_t index, UBool exec,const char* &name, char* par = NULL);     
    /* NFC performance */
    UPerfFunction* TestICU_NFC_NFD_Text();
    UPerfFunction* TestICU_NFC_NFC_Text();
    UPerfFunction* TestICU_NFC_Orig_Text();
    
    /* NFD performance */
    UPerfFunction* TestICU_NFD_NFD_Text();
    UPerfFunction* TestICU_NFD_NFC_Text();
    UPerfFunction* TestICU_NFD_Orig_Text();

    /* FCD performance */
    UPerfFunction* TestICU_FCD_NFD_Text();
    UPerfFunction* TestICU_FCD_NFC_Text();
    UPerfFunction* TestICU_FCD_Orig_Text();
    
    /*Win NFC performance */
    UPerfFunction* TestWin_NFC_NFD_Text();
    UPerfFunction* TestWin_NFC_NFC_Text();
    UPerfFunction* TestWin_NFC_Orig_Text();
    
    /* Win NFD performance */
    UPerfFunction* TestWin_NFD_NFD_Text();
    UPerfFunction* TestWin_NFD_NFC_Text();
    UPerfFunction* TestWin_NFD_Orig_Text();
    
    /* Quick check performance */
    UPerfFunction* TestQC_NFC_NFD_Text();
    UPerfFunction* TestQC_NFC_NFC_Text();
    UPerfFunction* TestQC_NFC_Orig_Text();

    UPerfFunction* TestQC_NFD_NFD_Text();
    UPerfFunction* TestQC_NFD_NFC_Text();
    UPerfFunction* TestQC_NFD_Orig_Text();

    UPerfFunction* TestQC_FCD_NFD_Text();
    UPerfFunction* TestQC_FCD_NFC_Text();
    UPerfFunction* TestQC_FCD_Orig_Text();

    /* IsNormalized performnace */
    UPerfFunction* TestIsNormalized_NFC_NFD_Text();
    UPerfFunction* TestIsNormalized_NFC_NFC_Text();
    UPerfFunction* TestIsNormalized_NFC_Orig_Text();

    UPerfFunction* TestIsNormalized_NFD_NFD_Text();
    UPerfFunction* TestIsNormalized_NFD_NFC_Text();
    UPerfFunction* TestIsNormalized_NFD_Orig_Text();

    UPerfFunction* TestIsNormalized_FCD_NFD_Text();
    UPerfFunction* TestIsNormalized_FCD_NFC_Text();
    UPerfFunction* TestIsNormalized_FCD_Orig_Text();

};

//---------------------------------------------------------------------------------------
// Platform / ICU version specific proto-types
//---------------------------------------------------------------------------------------


#if (U_ICU_VERSION_MAJOR_NUM > 1 ) || ((U_ICU_VERSION_MAJOR_NUM == 1 )&&(U_ICU_VERSION_MINOR_NUM > 8) && (U_ICU_VERSION_PATCHLEVEL_NUM >=1))

int32_t ICUNormNFD(const UChar* src, int32_t srcLen,UChar* dest, int32_t dstLen, int32_t options, UErrorCode* status) {
    return unorm_normalize(src,srcLen,UNORM_NFD, options,dest,dstLen,status);
}

int32_t ICUNormNFC(const UChar* src, int32_t srcLen,UChar* dest, int32_t dstLen, int32_t options, UErrorCode* status) {
    return unorm_normalize(src,srcLen,UNORM_NFC, options,dest,dstLen,status);
}

int32_t ICUNormNFKD(const UChar* src, int32_t srcLen,UChar* dest, int32_t dstLen, int32_t options, UErrorCode* status) {
    return unorm_normalize(src,srcLen,UNORM_NFKD, options,dest,dstLen,status);
}
int32_t ICUNormNFKC(const UChar* src, int32_t srcLen,UChar* dest, int32_t dstLen, int32_t options, UErrorCode* status) {
    return unorm_normalize(src,srcLen,UNORM_NFKC, options,dest,dstLen,status);
}

int32_t ICUNormFCD(const UChar* src, int32_t srcLen,UChar* dest, int32_t dstLen, int32_t options, UErrorCode* status) {
    return unorm_normalize(src,srcLen,UNORM_FCD, options,dest,dstLen,status);
}

int32_t ICUQuickCheck(const UChar* src,int32_t srcLen, UNormalizationMode mode, int32_t options, UErrorCode* status){
#if (U_ICU_VERSION_MAJOR_NUM > 2 ) || ((U_ICU_VERSION_MAJOR_NUM == 2 )&&(U_ICU_VERSION_MINOR_NUM >= 6))
    return unorm_quickCheckWithOptions(src,srcLen,mode, options, status);
#else
    return unorm_quickCheck(src,srcLen,mode,status);
#endif
}
int32_t ICUIsNormalized(const UChar* src,int32_t srcLen, UNormalizationMode mode, int32_t options, UErrorCode* status){
    return unorm_isNormalized(src,srcLen,mode,status);
}


#else

int32_t ICUNormNFD(const UChar* src, int32_t srcLen,UChar* dest, int32_t dstLen, int32_t options, UErrorCode* status) {
    return unorm_normalize(src,srcLen,UCOL_DECOMP_CAN, options,dest,dstLen,status);
}

int32_t ICUNormNFC(const UChar* src, int32_t srcLen,UChar* dest, int32_t dstLen, int32_t options, UErrorCode* status) {
    return unorm_normalize(src,srcLen,UCOL_COMPOSE_CAN, options,dest,dstLen,status);
}

int32_t ICUNormNFKD(const UChar* src, int32_t srcLen,UChar* dest, int32_t dstLen, int32_t options, UErrorCode* status) {
    return unorm_normalize(src,srcLen,UCOL_DECOMP_COMPAT, options,dest,dstLen,status);
}
int32_t ICUNormNFKC(const UChar* src, int32_t srcLen,UChar* dest, int32_t dstLen, int32_t options, UErrorCode* status) {
    return unorm_normalize(src,srcLen,UCOL_COMPOSE_COMPAT, options,dest,dstLen,status);
}

int32_t ICUNormFCD(const UChar* src, int32_t srcLen,UChar* dest, int32_t dstLen, int32_t options, UErrorCode* status) {
    return unorm_normalize(src,srcLen,UNORM_FCD, options,dest,dstLen,status);
}

int32_t ICUQuickCheck(const UChar* src,int32_t srcLen, UNormalizationMode mode, int32_t options, UErrorCode* status){
    return unorm_quickCheck(src,srcLen,mode,status);
}

int32_t ICUIsNormalized(const UChar* src,int32_t srcLen, UNormalizationMode mode, int32_t options, UErrorCode* status){
    return 0;
}
#endif

#if defined(U_WINDOWS)

int32_t WinNormNFD(const UChar* src, int32_t srcLen, UChar* dest, int32_t dstLen, int32_t options, UErrorCode* status) {
    return FoldStringW(MAP_COMPOSITE,src,srcLen,dest,dstLen);
}

int32_t WinNormNFC(const UChar* src, int32_t srcLen, UChar* dest, int32_t dstLen, int32_t options, UErrorCode* status) {
    return FoldStringW(MAP_PRECOMPOSED,src,srcLen,dest,dstLen);
}

int32_t WinNormNFKD(const UChar* src, int32_t srcLen, UChar* dest, int32_t dstLen, int32_t options, UErrorCode* status) {
    return FoldStringW(MAP_COMPOSITE+MAP_FOLDCZONE,src,srcLen,dest,dstLen);
}
int32_t WinNormNFKC(const UChar* src, int32_t srcLen, UChar* dest, int32_t dstLen, int32_t options, UErrorCode* status) {
    return FoldStringW(MAP_FOLDCZONE,src,srcLen,dest,dstLen);
}
#else
int32_t WinNormNFD(const UChar* src, int32_t srcLen, UChar* dest, int32_t dstLen, int32_t options, UErrorCode* status) {
    return 0 ;
}

int32_t WinNormNFC(const UChar* src, int32_t srcLen, UChar* dest, int32_t dstLen, int32_t options, UErrorCode* status) {
    return 0;
}

int32_t WinNormNFKD(const UChar* src, int32_t srcLen, UChar* dest, int32_t dstLen, int32_t options, UErrorCode* status) {
    return 0;
}
int32_t WinNormNFKC(const UChar* src, int32_t srcLen, UChar* dest, int32_t dstLen, int32_t options, UErrorCode* status) {
    return 0;
}
#endif


#endif // NORMPERF_H

