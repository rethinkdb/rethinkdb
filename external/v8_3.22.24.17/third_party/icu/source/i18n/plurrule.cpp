/*
*******************************************************************************
* Copyright (C) 2007-2010, International Business Machines Corporation and
* others. All Rights Reserved.
*******************************************************************************
*
* File PLURRULE.CPP
*
* Modification History:
*
*   Date        Name        Description
*******************************************************************************
*/


#include "unicode/uniset.h"
#include "unicode/utypes.h"
#include "unicode/ures.h"
#include "unicode/plurrule.h"
#include "cmemory.h"
#include "cstring.h"
#include "hash.h"
#include "mutex.h"
#include "plurrule_impl.h"
#include "putilimp.h"
#include "ucln_in.h"
#include "ustrfmt.h"
#include "locutil.h"

/*
// TODO(claireho): remove stdio
#include "stdio.h"
*/

#if !UCONFIG_NO_FORMATTING

U_NAMESPACE_BEGIN


#define ARRAY_SIZE(array) (int32_t)(sizeof array  / sizeof array[0])

static const UChar PLURAL_KEYWORD_ZERO[] = {LOW_Z,LOW_E,LOW_R,LOW_O, 0};
static const UChar PLURAL_KEYWORD_ONE[]={LOW_O,LOW_N,LOW_E,0};
static const UChar PLURAL_KEYWORD_TWO[]={LOW_T,LOW_W,LOW_O,0};
static const UChar PLURAL_KEYWORD_FEW[]={LOW_F,LOW_E,LOW_W,0};
static const UChar PLURAL_KEYWORD_MANY[]={LOW_M,LOW_A,LOW_N,LOW_Y,0};
static const UChar PLURAL_KEYWORD_OTHER[]={LOW_O,LOW_T,LOW_H,LOW_E,LOW_R,0};
static const UChar PLURAL_DEFAULT_RULE[]={LOW_O,LOW_T,LOW_H,LOW_E,LOW_R,COLON,SPACE,LOW_N,0};
static const UChar PK_IN[]={LOW_I,LOW_N,0};
static const UChar PK_NOT[]={LOW_N,LOW_O,LOW_T,0};
static const UChar PK_IS[]={LOW_I,LOW_S,0};
static const UChar PK_MOD[]={LOW_M,LOW_O,LOW_D,0};
static const UChar PK_AND[]={LOW_A,LOW_N,LOW_D,0};
static const UChar PK_OR[]={LOW_O,LOW_R,0};
static const UChar PK_VAR_N[]={LOW_N,0};
static const UChar PK_WITHIN[]={LOW_W,LOW_I,LOW_T,LOW_H,LOW_I,LOW_N,0};

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(PluralRules)
UOBJECT_DEFINE_RTTI_IMPLEMENTATION(PluralKeywordEnumeration)

PluralRules::PluralRules(UErrorCode& status)
:   UObject(),
    mRules(NULL)
{
    if (U_FAILURE(status)) {
        return;
    }
    mParser = new RuleParser();
    if (mParser==NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
}

PluralRules::PluralRules(const PluralRules& other)
: UObject(other),
    mRules(NULL),
    mParser(new RuleParser())
{
    *this=other;
}

PluralRules::~PluralRules() {
    delete mRules;
    delete mParser;
}

PluralRules*
PluralRules::clone() const {
    return new PluralRules(*this);
}

PluralRules&
PluralRules::operator=(const PluralRules& other) {
    if (this != &other) {
        delete mRules;
        if (other.mRules==NULL) {
            mRules = NULL;
        }
        else {
            mRules = new RuleChain(*other.mRules);
        }
        delete mParser;
        mParser = new RuleParser();
    }

    return *this;
}

PluralRules* U_EXPORT2
PluralRules::createRules(const UnicodeString& description, UErrorCode& status) {
    RuleChain   rules;

    if (U_FAILURE(status)) {
        return NULL;
    }
    PluralRules *newRules = new PluralRules(status);
    if ( (newRules != NULL)&& U_SUCCESS(status) ) {
        newRules->parseDescription((UnicodeString &)description, rules, status);
        if (U_SUCCESS(status)) {
            newRules->addRules(rules);
        }
    }
    if (U_FAILURE(status)) {
        delete newRules;
        return NULL;
    }
    else {
        return newRules;
    }
}

PluralRules* U_EXPORT2
PluralRules::createDefaultRules(UErrorCode& status) {
    return createRules(PLURAL_DEFAULT_RULE, status);
}

PluralRules* U_EXPORT2
PluralRules::forLocale(const Locale& locale, UErrorCode& status) {
    RuleChain   rChain;
    if (U_FAILURE(status)) {
        return NULL;
    }
    PluralRules *newObj = new PluralRules(status);
    if (newObj==NULL || U_FAILURE(status)) {
        return NULL;
    }
    UnicodeString locRule = newObj->getRuleFromResource(locale, status);
    if ((locRule.length() != 0) && U_SUCCESS(status)) {
        newObj->parseDescription(locRule, rChain, status);
        if (U_SUCCESS(status)) {
            newObj->addRules(rChain);
        }
    }
    if (U_FAILURE(status)||(locRule.length() == 0)) {
        // use default plural rule
        status = U_ZERO_ERROR;
        UnicodeString defRule = UnicodeString(PLURAL_DEFAULT_RULE);
        newObj->parseDescription(defRule, rChain, status);
        newObj->addRules(rChain);
    }
    
    return newObj;
}

UnicodeString
PluralRules::select(int32_t number) const {
    if (mRules == NULL) {
        return PLURAL_DEFAULT_RULE;
    }
    else {
        return mRules->select(number);
    }
}

UnicodeString
PluralRules::select(double number) const {
    if (mRules == NULL) {
        return PLURAL_DEFAULT_RULE;
    }
    else {
        return mRules->select(number);
    }
}

StringEnumeration*
PluralRules::getKeywords(UErrorCode& status) const {
    if (U_FAILURE(status))  return NULL;
    StringEnumeration* nameEnumerator = new PluralKeywordEnumeration(mRules, status);
    if (U_FAILURE(status))  return NULL;

    return nameEnumerator;
}


UBool
PluralRules::isKeyword(const UnicodeString& keyword) const {
    if ( keyword == PLURAL_KEYWORD_OTHER ) {
        return true;
    }
    else {
        if (mRules==NULL) {
            return false;
        }
        else {
            return mRules->isKeyword(keyword);
        }
    }
}

UnicodeString
PluralRules::getKeywordOther() const {
    return PLURAL_KEYWORD_OTHER;
}

UBool
PluralRules::operator==(const PluralRules& other) const  {
    int32_t limit;
    UBool sameList = TRUE;
    const UnicodeString *ptrKeyword;
    UErrorCode status= U_ZERO_ERROR;

    if ( this == &other ) {
        return TRUE;
    }
    StringEnumeration* myKeywordList = getKeywords(status);
    if (U_FAILURE(status)) {
        return FALSE;
    }
    StringEnumeration* otherKeywordList =other.getKeywords(status);
    if (U_FAILURE(status)) {
        return FALSE;
    }

    if (myKeywordList->count(status)!=otherKeywordList->count(status) ||
        U_FAILURE(status)) {
        sameList = FALSE;
    }
    else {
        myKeywordList->reset(status);
        if (U_FAILURE(status)) {
            return FALSE;
        }
        while (sameList && (ptrKeyword=myKeywordList->snext(status))!=NULL) {
            if (U_FAILURE(status) || !other.isKeyword(*ptrKeyword)) {
                sameList = FALSE;
            }
        }
        otherKeywordList->reset(status);
        if (U_FAILURE(status)) {
            return FALSE;
        }
        while (sameList && (ptrKeyword=otherKeywordList->snext(status))!=NULL) {
            if (U_FAILURE(status)) {
                return FALSE;
            }
            if (!this->isKeyword(*ptrKeyword))  {
                sameList = FALSE;
            }
        }
        delete myKeywordList;
        delete otherKeywordList;
        if (!sameList) {
            return FALSE;
        }
    }

    if ((limit=this->getRepeatLimit()) != other.getRepeatLimit()) {
        return FALSE;
    }
    UnicodeString myKeyword, otherKeyword;
    for (int32_t i=0; i<limit; ++i) {
        myKeyword = this->select(i);
        otherKeyword = other.select(i);
        if (myKeyword!=otherKeyword) {
            return FALSE;
        }
    }
    return TRUE;
}

void
PluralRules::parseDescription(UnicodeString& data, RuleChain& rules, UErrorCode &status)
{
    int32_t ruleIndex=0;
    UnicodeString token;
    tokenType type;
    tokenType prevType=none;
    RuleChain *ruleChain=NULL;
    AndConstraint *curAndConstraint=NULL;
    OrConstraint *orNode=NULL;
    RuleChain *lastChain=NULL;

    if (U_FAILURE(status)) {
        return;
    }
    UnicodeString ruleData = data.toLower();
    while (ruleIndex< ruleData.length()) {
        mParser->getNextToken(ruleData, &ruleIndex, token, type, status);
        if (U_FAILURE(status)) {
            return;
        }
        mParser->checkSyntax(prevType, type, status);
        if (U_FAILURE(status)) {
            return;
        }
        switch (type) {
        case tAnd:
            curAndConstraint = curAndConstraint->add();
            break;
        case tOr:
            lastChain = &rules;
            while (lastChain->next !=NULL) {
                lastChain = lastChain->next;
            }
            orNode=lastChain->ruleHeader;
            while (orNode->next != NULL) {
                orNode = orNode->next;
            }
            orNode->next= new OrConstraint();
            orNode=orNode->next;
            orNode->next=NULL;
            curAndConstraint = orNode->add();
            break;
        case tIs:
            curAndConstraint->rangeHigh=-1;
            break;
        case tNot:
            curAndConstraint->notIn=TRUE;
            break;
        case tIn:
            curAndConstraint->rangeHigh=PLURAL_RANGE_HIGH;
            curAndConstraint->integerOnly = TRUE;
            break;
        case tWithin:
            curAndConstraint->rangeHigh=PLURAL_RANGE_HIGH;
            break;
        case tNumber:
            if ( (curAndConstraint->op==AndConstraint::MOD)&&
                 (curAndConstraint->opNum == -1 ) ) {
                curAndConstraint->opNum=getNumberValue(token);
            }
            else {
                if (curAndConstraint->rangeLow == -1) {
                    curAndConstraint->rangeLow=getNumberValue(token);
                }
                else {
                    curAndConstraint->rangeHigh=getNumberValue(token);
                }
            }
            break;
        case tMod:
            curAndConstraint->op=AndConstraint::MOD;
            break;
        case tKeyword:
            if (ruleChain==NULL) {
                ruleChain = &rules;
            }
            else {
                while (ruleChain->next!=NULL){
                    ruleChain=ruleChain->next;
                }
                ruleChain=ruleChain->next=new RuleChain();
            }
            orNode = ruleChain->ruleHeader = new OrConstraint();
            curAndConstraint = orNode->add();
            ruleChain->keyword = token;
            break;
        default:
            break;
        }
        prevType=type;
    }
}

int32_t
PluralRules::getNumberValue(const UnicodeString& token) const {
    int32_t i;
    char digits[128];

    i = token.extract(0, token.length(), digits, ARRAY_SIZE(digits), US_INV);
    digits[i]='\0';

    return((int32_t)atoi(digits));
}


void
PluralRules::getNextLocale(const UnicodeString& localeData, int32_t* curIndex, UnicodeString& localeName) {
    int32_t i=*curIndex;

    localeName.remove();
    while (i< localeData.length()) {
       if ( (localeData.charAt(i)!= SPACE) && (localeData.charAt(i)!= COMMA) ) {
           break;
       }
       i++;
    }

    while (i< localeData.length()) {
       if ( (localeData.charAt(i)== SPACE) || (localeData.charAt(i)== COMMA) ) {
           break;
       }
       localeName+=localeData.charAt(i++);
    }
    *curIndex=i;
}


int32_t
PluralRules::getRepeatLimit() const {
    if (mRules!=NULL) {
        return mRules->getRepeatLimit();
    }
    else {
        return 0;
    }
}


void
PluralRules::addRules(RuleChain& rules) {
    RuleChain *newRule = new RuleChain(rules);
    this->mRules=newRule;
    newRule->setRepeatLimit();
}

UnicodeString
PluralRules::getRuleFromResource(const Locale& locale, UErrorCode& errCode) {
    UnicodeString emptyStr;
    
    if (U_FAILURE(errCode)) {
        return emptyStr;
    }
    UResourceBundle *rb=ures_openDirect(NULL, "plurals", &errCode);
    if(U_FAILURE(errCode)) {
        /* total failure, not even root could be opened */
        return emptyStr;
    }
    UResourceBundle *locRes=ures_getByKey(rb, "locales", NULL, &errCode);
    if(U_FAILURE(errCode)) {
        ures_close(rb);
        return emptyStr;
    }   
    int32_t resLen=0;
    const char *curLocaleName=locale.getName();
    const UChar* s = ures_getStringByKey(locRes, curLocaleName, &resLen, &errCode);

    if (s == NULL) {
        // Check parent locales.
        UErrorCode status = U_ZERO_ERROR;
        char parentLocaleName[ULOC_FULLNAME_CAPACITY];
        const char *curLocaleName=locale.getName();
        int32_t localeNameLen=0;
        uprv_strcpy(parentLocaleName, curLocaleName);
        
        while ((localeNameLen=uloc_getParent(parentLocaleName, parentLocaleName, 
                                       ULOC_FULLNAME_CAPACITY, &status)) > 0) {
            resLen=0;
            s = ures_getStringByKey(locRes, parentLocaleName, &resLen, &status);
            if (s != NULL) {
                errCode = U_ZERO_ERROR;
                break;
            }
            status = U_ZERO_ERROR;
        }
    }
    if (s==NULL) {
        ures_close(locRes);
        ures_close(rb);
        return emptyStr;
    }
    
    char setKey[256];
    UChar result[256];
    u_UCharsToChars(s, setKey, resLen + 1);
    // printf("\n PluralRule: %s\n", setKey);
    

    UResourceBundle *ruleRes=ures_getByKey(rb, "rules", NULL, &errCode);
    if(U_FAILURE(errCode)) {
        ures_close(locRes);
        ures_close(rb);
        return emptyStr;
    }
    resLen=0;
    UResourceBundle *setRes = ures_getByKey(ruleRes, setKey, NULL, &errCode);
    if (U_FAILURE(errCode)) {
        ures_close(ruleRes);
        ures_close(locRes);
        ures_close(rb);
        return emptyStr;
    }

    int32_t numberKeys = ures_getSize(setRes);
    char *key=NULL;
    int32_t len=0;
    for(int32_t i=0; i<numberKeys; ++i) {
        int32_t keyLen;
        resLen=0;
        s=ures_getNextString(setRes, &resLen, (const char**)&key, &errCode);
        keyLen = (int32_t)uprv_strlen(key);
        u_charsToUChars(key, result+len, keyLen);
        len += keyLen;
        result[len++]=COLON;
        uprv_memcpy(result+len, s, resLen*sizeof(UChar));
        len += resLen;
        result[len++]=SEMI_COLON;
    }
    result[len++]=0;
    u_UCharsToChars(result, setKey, len);
    // printf(" Rule: %s\n", setKey);

    ures_close(setRes);
    ures_close(ruleRes);
    ures_close(locRes);
    ures_close(rb);
    return UnicodeString(result);
    
}

AndConstraint::AndConstraint() {
    op = AndConstraint::NONE;
    opNum=-1;
    rangeLow=-1;
    rangeHigh=-1;
    notIn=FALSE;
    integerOnly=FALSE;
    next=NULL;
}


AndConstraint::AndConstraint(const AndConstraint& other) {
    this->op = other.op;
    this->opNum=other.opNum;
    this->rangeLow=other.rangeLow;
    this->rangeHigh=other.rangeHigh;
    this->integerOnly=other.integerOnly;
    this->notIn=other.notIn;
    if (other.next==NULL) {
        this->next=NULL;
    }
    else {
        this->next = new AndConstraint(*other.next);
    }
}

AndConstraint::~AndConstraint() {
    if (next!=NULL) {
        delete next;
    }
}


UBool
AndConstraint::isFulfilled(double number) {
    UBool result=TRUE;
    double value=number;
    
    if ( op == MOD ) {
        value = (int32_t)value % opNum;
    }
    if ( rangeHigh == -1 ) {
        if ( rangeLow == -1 ) {
            result = TRUE; // empty rule
        }
        else {
            if ( value == rangeLow ) {
                result = TRUE;
            }
            else {
                result = FALSE;
            }
        }
    }
    else {
        if ((rangeLow <= value) && (value <= rangeHigh)) {
            if (integerOnly) {
                if ( value != (int32_t)value) {
                    result = FALSE;
                }
                else {
                    result = TRUE;
                }
            }
            else {
                result = TRUE;
            }
        }
        else {
            result = FALSE;
        }
    }
    if (notIn) {
        return !result;
    }
    else {
        return result;
    }
}

int32_t
AndConstraint::updateRepeatLimit(int32_t maxLimit) {
    
    if ( op == MOD ) {
        return uprv_max(opNum, maxLimit);
    }
    else {
        if ( rangeHigh == -1 ) {
            return uprv_max(rangeLow, maxLimit);
        }
        else{
            return uprv_max(rangeHigh, maxLimit);
        }
    }
}


AndConstraint*
AndConstraint::add()
{
    this->next = new AndConstraint();
    return this->next;
}

OrConstraint::OrConstraint() {
    childNode=NULL;
    next=NULL;
}

OrConstraint::OrConstraint(const OrConstraint& other) {
    if ( other.childNode == NULL ) {
        this->childNode = NULL;
    }
    else {
        this->childNode = new AndConstraint(*(other.childNode));
    }
    if (other.next == NULL ) {
        this->next = NULL;
    }
    else {
        this->next = new OrConstraint(*(other.next));
    }
}

OrConstraint::~OrConstraint() {
    if (childNode!=NULL) {
        delete childNode;
    }
    if (next!=NULL) {
        delete next;
    }
}

AndConstraint*
OrConstraint::add()
{
    OrConstraint *curOrConstraint=this;
    {
        while (curOrConstraint->next!=NULL) {
            curOrConstraint = curOrConstraint->next;
        }
        curOrConstraint->next = NULL;
        curOrConstraint->childNode = new AndConstraint();
    }
    return curOrConstraint->childNode;
}

UBool
OrConstraint::isFulfilled(double number) {
    OrConstraint* orRule=this;
    UBool result=FALSE;
    
    while (orRule!=NULL && !result) {
        result=TRUE;
        AndConstraint* andRule = orRule->childNode;
        while (andRule!=NULL && result) {
            result = andRule->isFulfilled(number);
            andRule=andRule->next;
        }
        orRule = orRule->next;
    }
    
    return result;
}


RuleChain::RuleChain() {
    ruleHeader=NULL;
    next = NULL;
    repeatLimit=0;
}

RuleChain::RuleChain(const RuleChain& other) {
    this->repeatLimit = other.repeatLimit;
    this->keyword=other.keyword;
    if (other.ruleHeader != NULL) {
        this->ruleHeader = new OrConstraint(*(other.ruleHeader));
    }
    else {
        this->ruleHeader = NULL;
    }
    if (other.next != NULL ) {
        this->next = new RuleChain(*other.next);
    }
    else
    {
        this->next = NULL;
    }
}

RuleChain::~RuleChain() {
    if (next != NULL) {
        delete next;
    }
    if ( ruleHeader != NULL ) {
        delete ruleHeader;
    }
}

UnicodeString
RuleChain::select(double number) const {
   
   if ( ruleHeader != NULL ) {
       if (ruleHeader->isFulfilled(number)) {
           return keyword;
       }
   }
   if ( next != NULL ) {
       return next->select(number);
   }
   else {
       return PLURAL_KEYWORD_OTHER;
   }

}

void
RuleChain::dumpRules(UnicodeString& result) {
    UChar digitString[16];
    
    if ( ruleHeader != NULL ) {
        result +=  keyword;
        OrConstraint* orRule=ruleHeader;
        while ( orRule != NULL ) {
            AndConstraint* andRule=orRule->childNode;
            while ( andRule != NULL ) {
                if ( (andRule->op==AndConstraint::NONE) && (andRule->rangeHigh==-1) ) {
                    result += UNICODE_STRING_SIMPLE(" n is ");
                    if (andRule->notIn) {
                        result += UNICODE_STRING_SIMPLE("not ");
                    }
                    uprv_itou(digitString,16, andRule->rangeLow,10,0);
                    result += UnicodeString(digitString);
                }
                else {
                    if (andRule->op==AndConstraint::MOD) {
                        result += UNICODE_STRING_SIMPLE("  n mod ");
                        uprv_itou(digitString,16, andRule->opNum,10,0);
                        result += UnicodeString(digitString);
                    }
                    else {
                        result += UNICODE_STRING_SIMPLE("  n ");
                    }
                    if (andRule->rangeHigh==-1) {
                        if (andRule->notIn) {
                            result += UNICODE_STRING_SIMPLE(" is not ");
                            uprv_itou(digitString,16, andRule->rangeLow,10,0);
                            result += UnicodeString(digitString);
                        }
                        else {
                            result += UNICODE_STRING_SIMPLE(" is ");
                            uprv_itou(digitString,16, andRule->rangeLow,10,0);
                            result += UnicodeString(digitString);
                        }
                    }
                    else {
                        if (andRule->notIn) {
                            if ( andRule->integerOnly ) {
                                result += UNICODE_STRING_SIMPLE("  not in ");
                            }
                            else {
                                result += UNICODE_STRING_SIMPLE("  not within ");
                            }
                            uprv_itou(digitString,16, andRule->rangeLow,10,0);
                            result += UnicodeString(digitString);
                            result += UNICODE_STRING_SIMPLE(" .. ");
                            uprv_itou(digitString,16, andRule->rangeHigh,10,0);
                            result += UnicodeString(digitString);
                        }
                        else {
                            if ( andRule->integerOnly ) {
                                result += UNICODE_STRING_SIMPLE(" in ");
                            }
                            else {
                                result += UNICODE_STRING_SIMPLE(" within ");
                            }
                            uprv_itou(digitString,16, andRule->rangeLow,10,0);
                            result += UnicodeString(digitString);
                            result += UNICODE_STRING_SIMPLE(" .. ");
                            uprv_itou(digitString,16, andRule->rangeHigh,10,0);
                        }
                    }
                }
                if ( (andRule=andRule->next) != NULL) {
                    result += PK_AND;
                }
            }
            if ( (orRule = orRule->next) != NULL ) {
                result += PK_OR;
            }
        }
    }
    if ( next != NULL ) {
        next->dumpRules(result);
    }
}

int32_t
RuleChain::getRepeatLimit () {
    return repeatLimit;
}

void
RuleChain::setRepeatLimit () {
    int32_t limit=0;

    if ( next != NULL ) {
        next->setRepeatLimit();
        limit = next->repeatLimit;
    }

    if ( ruleHeader != NULL ) {
        OrConstraint* orRule=ruleHeader;
        while ( orRule != NULL ) {
            AndConstraint* andRule=orRule->childNode;
            while ( andRule != NULL ) {
                limit = andRule->updateRepeatLimit(limit);
                andRule = andRule->next;
            }
            orRule = orRule->next;
        }
    }
    repeatLimit = limit;
}

UErrorCode
RuleChain::getKeywords(int32_t capacityOfKeywords, UnicodeString* keywords, int32_t& arraySize) const {
    if ( arraySize < capacityOfKeywords-1 ) {
        keywords[arraySize++]=keyword;
    }
    else {
        return U_BUFFER_OVERFLOW_ERROR;
    }

    if ( next != NULL ) {
        return next->getKeywords(capacityOfKeywords, keywords, arraySize);
    }
    else {
        return U_ZERO_ERROR;
    }
}

UBool
RuleChain::isKeyword(const UnicodeString& keywordParam) const {
    if ( keyword == keywordParam ) {
        return TRUE;
    }

    if ( next != NULL ) {
        return next->isKeyword(keywordParam);
    }
    else {
        return FALSE;
    }
}


RuleParser::RuleParser() {
    UErrorCode err=U_ZERO_ERROR;
    const UnicodeString idStart=UNICODE_STRING_SIMPLE("[[a-z]]");
    const UnicodeString idContinue=UNICODE_STRING_SIMPLE("[[a-z][A-Z][_][0-9]]");
    idStartFilter = new UnicodeSet(idStart, err);
    idContinueFilter = new UnicodeSet(idContinue, err);
}

RuleParser::~RuleParser() {
    delete idStartFilter;
    delete idContinueFilter;
}

void
RuleParser::checkSyntax(tokenType prevType, tokenType curType, UErrorCode &status)
{
    if (U_FAILURE(status)) {
        return;
    }
    switch(prevType) {
    case none:
    case tSemiColon:
        if (curType!=tKeyword) {
            status = U_UNEXPECTED_TOKEN;
        }
        break;
    case tVariableN :
        if (curType != tIs && curType != tMod && curType != tIn && 
            curType != tNot && curType != tWithin) {
            status = U_UNEXPECTED_TOKEN;
        }
        break;
    case tZero:
    case tOne:
    case tTwo:
    case tFew:
    case tMany:
    case tOther:
    case tKeyword:
        if (curType != tColon) {
            status = U_UNEXPECTED_TOKEN;
        }
        break;
    case tColon :
        if (curType != tVariableN) {
            status = U_UNEXPECTED_TOKEN;
        }
        break;
    case tIs:
        if ( curType != tNumber && curType != tNot) {
            status = U_UNEXPECTED_TOKEN;
        }
        break;
    case tNot:
        if (curType != tNumber && curType != tIn && curType != tWithin) {
            status = U_UNEXPECTED_TOKEN;
        }
        break;
    case tMod:
    case tDot:
    case tIn:
    case tWithin:
    case tAnd:
    case tOr:
        if (curType != tNumber && curType != tVariableN) {
            status = U_UNEXPECTED_TOKEN;
        }
        break;
    case tNumber:
        if (curType != tDot && curType != tSemiColon && curType != tIs && curType != tNot &&
            curType != tIn && curType != tWithin && curType != tAnd && curType != tOr)
        {
            status = U_UNEXPECTED_TOKEN;
        }
        break;
    default:
        status = U_UNEXPECTED_TOKEN;
        break;
    }
}

void
RuleParser::getNextToken(const UnicodeString& ruleData,
                         int32_t *ruleIndex,
                         UnicodeString& token,
                         tokenType& type,
                         UErrorCode &status)
{
    int32_t curIndex= *ruleIndex;
    UChar ch;
    tokenType prevType=none;

    if (U_FAILURE(status)) {
        return;
    }
    while (curIndex<ruleData.length()) {
        ch = ruleData.charAt(curIndex);
        if ( !inRange(ch, type) ) {
            status = U_ILLEGAL_CHARACTER;
            return;
        }
        switch (type) {
        case tSpace:
            if ( *ruleIndex != curIndex ) { // letter
                token=UnicodeString(ruleData, *ruleIndex, curIndex-*ruleIndex);
                *ruleIndex=curIndex;
                type=prevType;
                getKeyType(token, type, status);
                return;
            }
            else {
                *ruleIndex=*ruleIndex+1;
            }
            break; // consective space
        case tColon:
        case tSemiColon:
            if ( *ruleIndex != curIndex ) {
                token=UnicodeString(ruleData, *ruleIndex, curIndex-*ruleIndex);
                *ruleIndex=curIndex;
                type=prevType;
                getKeyType(token, type, status);
                return;
            }
            else {
                *ruleIndex=curIndex+1;
                return;
            }
        case tLetter:
             if ((type==prevType)||(prevType==none)) {
                prevType=type;
                break;
             }
             break;
        case tNumber:
             if ((type==prevType)||(prevType==none)) {
                prevType=type;
                break;
             }
             else {
                *ruleIndex=curIndex+1;
                return;
             }
         case tDot:
             if (prevType==none) {  // first dot
                prevType=type;
                continue;
             }
             else {
                 if ( *ruleIndex != curIndex ) {
                    token=UnicodeString(ruleData, *ruleIndex, curIndex-*ruleIndex);
                    *ruleIndex=curIndex;  // letter
                    type=prevType;
                    getKeyType(token, type, status);
                    return;
                 }
                 else {  // two consective dots
                    *ruleIndex=curIndex+2;
                    return;
                 }
             }
             break;
         default:
             status = U_UNEXPECTED_TOKEN;
             return;
        }
        curIndex++;
    }
    if ( curIndex>=ruleData.length() ) {
        if ( (type == tLetter)||(type == tNumber) ) {
            token=UnicodeString(ruleData, *ruleIndex, curIndex-*ruleIndex);
            getKeyType(token, type, status);
            if (U_FAILURE(status)) {
                return;
            }
        }
        *ruleIndex = ruleData.length();
    }
}

UBool
RuleParser::inRange(UChar ch, tokenType& type) {
    if ((ch>=CAP_A) && (ch<=CAP_Z)) {
        // we assume all characters are in lower case already.
        return FALSE;
    }
    if ((ch>=LOW_A) && (ch<=LOW_Z)) {
        type = tLetter;
        return TRUE;
    }
    if ((ch>=U_ZERO) && (ch<=U_NINE)) {
        type = tNumber;
        return TRUE;
    }
    switch (ch) {
    case COLON:
        type = tColon;
        return TRUE;
    case SPACE:
        type = tSpace;
        return TRUE;
    case SEMI_COLON:
        type = tSemiColon;
        return TRUE;
    case DOT:
        type = tDot;
        return TRUE;
    default :
        type = none;
        return FALSE;
    }
}


void
RuleParser::getKeyType(const UnicodeString& token, tokenType& keyType, UErrorCode &status)
{
    if (U_FAILURE(status)) {
        return;
    }
    if ( keyType==tNumber) {
    }
    else if (token==PK_VAR_N) {
        keyType = tVariableN;
    }
    else if (token==PK_IS) {
        keyType = tIs;
    }
    else if (token==PK_AND) {
        keyType = tAnd;
    }
    else if (token==PK_IN) {
        keyType = tIn;
    }
    else if (token==PK_WITHIN) {
        keyType = tWithin;
    }
    else if (token==PK_NOT) {
        keyType = tNot;
    }
    else if (token==PK_MOD) {
        keyType = tMod;
    }
    else if (token==PK_OR) {
        keyType = tOr;
    }
    else if ( isValidKeyword(token) ) {
        keyType = tKeyword;
    }
    else {
        status = U_UNEXPECTED_TOKEN;
    }
}

UBool
RuleParser::isValidKeyword(const UnicodeString& token) {
    if ( token.length()==0 ) {
        return FALSE;
    }
    if ( idStartFilter->contains(token.charAt(0) )==TRUE ) {
        int32_t i;
        for (i=1; i< token.length(); i++) {
            if (idContinueFilter->contains(token.charAt(i))== FALSE) {
                return FALSE;
            }
        }
        return TRUE;
    }
    else {
        return FALSE;
    }
}

PluralKeywordEnumeration::PluralKeywordEnumeration(RuleChain *header, UErrorCode& status) :
fKeywordNames(status)
{
    RuleChain *node=header;
    UBool  addKeywordOther=true;

    if (U_FAILURE(status)) {
        return;
    }
    pos=0;
    fKeywordNames.removeAllElements();
    while(node!=NULL) {
        fKeywordNames.addElement(new UnicodeString(node->keyword), status);
        if (U_FAILURE(status)) {
            return;
        }
        if (node->keyword == PLURAL_KEYWORD_OTHER) {
            addKeywordOther= false;
        }
        node=node->next;
    }
    
    if (addKeywordOther) {
        fKeywordNames.addElement(new UnicodeString(PLURAL_KEYWORD_OTHER), status);
        if (U_FAILURE(status)) {
            return;
        }
    }
}

const UnicodeString*
PluralKeywordEnumeration::snext(UErrorCode& status) {
    if (U_SUCCESS(status) && pos < fKeywordNames.size()) {
        return (const UnicodeString*)fKeywordNames.elementAt(pos++);
    }
    return NULL;
}

void
PluralKeywordEnumeration::reset(UErrorCode& /*status*/) {
    pos=0;
}

int32_t
PluralKeywordEnumeration::count(UErrorCode& /*status*/) const {
       return fKeywordNames.size();
}

PluralKeywordEnumeration::~PluralKeywordEnumeration() {
    UnicodeString *s;
    for (int32_t i=0; i<fKeywordNames.size(); ++i) {
        if ((s=(UnicodeString *)fKeywordNames.elementAt(i))!=NULL) {
            delete s;
        }
    }
}

U_NAMESPACE_END


#endif /* #if !UCONFIG_NO_FORMATTING */

//eof
