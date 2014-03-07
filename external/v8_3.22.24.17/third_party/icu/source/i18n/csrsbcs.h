/*
 **********************************************************************
 *   Copyright (C) 2005-2009, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 **********************************************************************
 */

#ifndef __CSRSBCS_H
#define __CSRSBCS_H

#include "unicode/uobject.h"

#if !UCONFIG_NO_CONVERSION

#include "csrecog.h"

U_NAMESPACE_BEGIN

class NGramParser : public UMemory
{
private:
    int32_t byteIndex;
    int32_t ngram;

    const int32_t *ngramList;
    const uint8_t *charMap;

    int32_t ngramCount;
    int32_t hitCount;

public:
    NGramParser(const int32_t *theNgramList, const uint8_t *theCharMap);

private:
    /*
    * Binary search for value in table, which must have exactly 64 entries.
    */
    int32_t search(const int32_t *table, int32_t value);

    void lookup(int32_t thisNgram);
    void addByte(int32_t b);
    int32_t nextByte(InputText *det);

public:
    int32_t parse(InputText *det);

};

class CharsetRecog_sbcs : public CharsetRecognizer
{
protected:
    UBool haveC1Bytes;

public:
    CharsetRecog_sbcs();

    virtual ~CharsetRecog_sbcs();

    virtual const char *getName() const = 0;

    virtual int32_t match(InputText *det) = 0;

    int32_t match_sbcs(InputText *det, const int32_t ngrams[], const uint8_t charMap[]);
};

class CharsetRecog_8859_1 : public CharsetRecog_sbcs
{
public:
    virtual ~CharsetRecog_8859_1();

    const char *getName() const;
};

class CharsetRecog_8859_2 : public CharsetRecog_sbcs
{
public:
    virtual ~CharsetRecog_8859_2();

    const char *getName() const;
};

class CharsetRecog_8859_5 : public CharsetRecog_sbcs
{
public:
    virtual ~CharsetRecog_8859_5();

    const char *getName() const;
};

class CharsetRecog_8859_6 : public CharsetRecog_sbcs
{
public:
    virtual ~CharsetRecog_8859_6();

    const char *getName() const;
};

class CharsetRecog_8859_7 : public CharsetRecog_sbcs
{
public:
    virtual ~CharsetRecog_8859_7();

    const char *getName() const;
};

class CharsetRecog_8859_8 : public CharsetRecog_sbcs
{
public:
    virtual ~CharsetRecog_8859_8();
	
    virtual const char *getName() const;
};

class CharsetRecog_8859_9 : public CharsetRecog_sbcs
{
public:
    virtual ~CharsetRecog_8859_9();

    const char *getName() const;
};

class CharsetRecog_8859_1_en : public CharsetRecog_8859_1
{
public:
    virtual ~CharsetRecog_8859_1_en();

    const char *getLanguage() const;

    int32_t match(InputText *textIn);
};

class CharsetRecog_8859_1_da : public CharsetRecog_8859_1
{
public:
    virtual ~CharsetRecog_8859_1_da();

    const char *getLanguage() const;

    int32_t match(InputText *textIn);
};

class CharsetRecog_8859_1_de : public CharsetRecog_8859_1
{
public:
    virtual ~CharsetRecog_8859_1_de();

    const char *getLanguage() const;

    int32_t match(InputText *textIn);
};

class CharsetRecog_8859_1_es : public CharsetRecog_8859_1
{
public:
    virtual ~CharsetRecog_8859_1_es();

    const char *getLanguage() const;

    int32_t match(InputText *textIn);
};

class CharsetRecog_8859_1_fr : public CharsetRecog_8859_1
{
public:
    virtual ~CharsetRecog_8859_1_fr();

    const char *getLanguage() const;

    int32_t match(InputText *textIn);
};

class CharsetRecog_8859_1_it : public CharsetRecog_8859_1
{
public:
    virtual ~CharsetRecog_8859_1_it();

    const char *getLanguage() const;

    int32_t match(InputText *textIn);
};

class CharsetRecog_8859_1_nl : public CharsetRecog_8859_1
{
public:
    virtual ~CharsetRecog_8859_1_nl();

    const char *getLanguage() const;

    int32_t match(InputText *textIn);
};

class CharsetRecog_8859_1_no : public CharsetRecog_8859_1
{
public:
    virtual ~CharsetRecog_8859_1_no();

    const char *getLanguage() const;

    int32_t match(InputText *textIn);
};

class CharsetRecog_8859_1_pt : public CharsetRecog_8859_1
{
public:
    virtual ~CharsetRecog_8859_1_pt();

    const char *getLanguage() const;

    int32_t match(InputText *textIn);
};

class CharsetRecog_8859_1_sv : public CharsetRecog_8859_1
{
public:
    virtual ~CharsetRecog_8859_1_sv();

    const char *getLanguage() const;

    int32_t match(InputText *textIn);
};

class CharsetRecog_8859_2_cs : public CharsetRecog_8859_2
{
public:
    virtual ~CharsetRecog_8859_2_cs();

    const char *getLanguage() const;

    int32_t match(InputText *textIn);
};

class CharsetRecog_8859_2_hu : public CharsetRecog_8859_2
{
public:
    virtual ~CharsetRecog_8859_2_hu();

    const char *getLanguage() const;

    int32_t match(InputText *textIn);
};

class CharsetRecog_8859_2_pl : public CharsetRecog_8859_2
{
public:
    virtual ~CharsetRecog_8859_2_pl();

    const char *getLanguage() const;

    int32_t match(InputText *textIn);
};

class CharsetRecog_8859_2_ro : public CharsetRecog_8859_2
{
public:
    virtual ~CharsetRecog_8859_2_ro();

    const char *getLanguage() const;

    int32_t match(InputText *textIn);
};

class CharsetRecog_8859_5_ru : public CharsetRecog_8859_5
{
public:
    virtual ~CharsetRecog_8859_5_ru();

    const char *getLanguage() const;

    int32_t match(InputText *textIn);
};

class CharsetRecog_8859_6_ar : public CharsetRecog_8859_6
{
public:
    virtual ~CharsetRecog_8859_6_ar();

    const char *getLanguage() const;

    int32_t match(InputText *textIn);
};

class CharsetRecog_8859_7_el : public CharsetRecog_8859_7
{
public:
    virtual ~CharsetRecog_8859_7_el();

    const char *getLanguage() const;

    int32_t match(InputText *textIn);
};

class CharsetRecog_8859_8_I_he : public CharsetRecog_8859_8
{
public:
    virtual ~CharsetRecog_8859_8_I_he();
	
    const char *getName() const;

    const char *getLanguage() const;

    int32_t match(InputText *textIn);
};

class CharsetRecog_8859_8_he : public CharsetRecog_8859_8
{
public:
    virtual ~CharsetRecog_8859_8_he ();

    const char *getLanguage() const;

    int32_t match(InputText *textIn);
};

class CharsetRecog_8859_9_tr : public CharsetRecog_8859_9
{
public:
    virtual ~CharsetRecog_8859_9_tr ();

    const char *getLanguage() const;

    int32_t match(InputText *textIn);
};

class CharsetRecog_windows_1256 : public CharsetRecog_sbcs
{
public:
    virtual ~CharsetRecog_windows_1256();

    const char *getName() const;

    const char *getLanguage() const;

    int32_t match(InputText *textIn);
};

class CharsetRecog_windows_1251 : public CharsetRecog_sbcs
{
public:
    virtual ~CharsetRecog_windows_1251();

    const char *getName() const;

    const char *getLanguage() const;

    int32_t match(InputText *textIn);
};


class CharsetRecog_KOI8_R : public CharsetRecog_sbcs
{
public:
    virtual ~CharsetRecog_KOI8_R();

    const char *getName() const;

    const char *getLanguage() const;

    int32_t match(InputText *textIn);
};

class CharsetRecog_IBM424_he : public CharsetRecog_sbcs
{
public:
    virtual ~CharsetRecog_IBM424_he();

    const char *getLanguage() const;
};

class CharsetRecog_IBM424_he_rtl : public CharsetRecog_IBM424_he {
public:
    virtual ~CharsetRecog_IBM424_he_rtl();
    
    const char *getName() const;
    
    int32_t match(InputText *textIn);
};

class CharsetRecog_IBM424_he_ltr : public CharsetRecog_IBM424_he {
    virtual ~CharsetRecog_IBM424_he_ltr();
    
    const char *getName() const;
    
    int32_t match(InputText *textIn);
};

class CharsetRecog_IBM420_ar : public CharsetRecog_sbcs
{
public:
    virtual ~CharsetRecog_IBM420_ar();

    const char *getLanguage() const;
    
protected:
    void matchInit(InputText *textIn);
    void matchFinish(InputText *textIn);
    
private:
    uint8_t *prev_fInputBytes;
    int32_t prev_fInputBytesLength;
    UBool deleteBuffer;
    
    UBool isLamAlef(uint8_t b);
    uint8_t *unshapeLamAlef(const uint8_t *inputBytes, int32_t inputBytesLength, int32_t &length);
    uint8_t *unshape(const uint8_t *inputBytes, int32_t inputBytesLength, int32_t &length);
};

class CharsetRecog_IBM420_ar_rtl : public CharsetRecog_IBM420_ar {
public:
    virtual ~CharsetRecog_IBM420_ar_rtl();
    
    const char *getName() const;
    
    int32_t match(InputText *textIn);
};

class CharsetRecog_IBM420_ar_ltr : public CharsetRecog_IBM420_ar {
    virtual ~CharsetRecog_IBM420_ar_ltr();
    
    const char *getName() const;
    
    int32_t match(InputText *textIn);
};

U_NAMESPACE_END

#endif
#endif /* __CSRSBCS_H */
