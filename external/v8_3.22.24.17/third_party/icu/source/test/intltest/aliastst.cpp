/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 2005-2006, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "aliastst.h"
#include "unicode/calendar.h"
#include "unicode/smpdtfmt.h"
#include "unicode/datefmt.h"
#include "unicode/unistr.h"
#include "unicode/coll.h"
#include "unicode/resbund.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

const char* _LOCALE_ALIAS[][2] = {
    {"in", "id"},
    {"in_ID", "id_ID"},
    {"iw", "he"},
    {"iw_IL", "he_IL"},
    {"ji", "yi"},
    {"en_BU", "en_MM"},
    {"en_DY", "en_BJ"},
    {"en_HV", "en_BF"},
    {"en_NH", "en_VU"},
    {"en_RH", "en_ZW"},
    {"en_TP", "en_TL"},
    {"en_ZR", "en_CD"}
};

const int _LOCALE_NUMBER = 12;

void LocaleAliasTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ ){
    switch (index) {
        TESTCASE(0, TestCalendar);
        TESTCASE(1, TestDateFormat);
        TESTCASE(2, TestCollation);
        TESTCASE(3, TestULocale);
        TESTCASE(4, TestUResourceBundle);
        TESTCASE(5, TestDisplayName);
        // keep the last index in sync with the condition in default:

        default:
            if (index <= 5) { // keep this in sync with the last index!
                name = "(test omitted by !UCONFIG_NO_FORMATTING)";
            } else {
                name = "";
            }
            break; //needed to end loop
    }
}

void LocaleAliasTest::TestCalendar() {
#if !UCONFIG_NO_FORMATTING
    UErrorCode status = U_ZERO_ERROR;
    for (int i=0; i<_LOCALE_NUMBER; i++) {
        Locale oldLoc(_LOCALE_ALIAS[i][0]);
        Locale newLoc(_LOCALE_ALIAS[i][1]);
        if(!isLocaleAvailable(_LOCALE_ALIAS[i][1])){
            logln(UnicodeString(newLoc.getName())+" is not available. Skipping!");
            continue;
        }
        logln("\nold locale:%s   new locale:%s",oldLoc.getName(),newLoc.getName());
        Calendar* c1 = Calendar::createInstance(oldLoc, status);
        Calendar* c2 = Calendar::createInstance(newLoc, status);

        //Test function "getLocale(ULocale.VALID_LOCALE)"
        const char* l1 = c1->getLocaleID(ULOC_VALID_LOCALE, status);
        const char* l2 = c2->getLocaleID(ULOC_VALID_LOCALE, status);
        if (strcmp(newLoc.getName(), l1)!=0) {
            errln("CalendarTest: newLoc!=l1: newLoc= "+UnicodeString(newLoc.getName()) +" l1= "+UnicodeString(l1));
        }
        if (strcmp(l1, l2)!=0) {
            errln("CalendarTest: l1!=l2: l1= "+UnicodeString(l1) +" l2= "+UnicodeString(l2));
        }
        if(!(c1==c2)){
            errln("CalendarTest: c1!=c2.  newLoc= "+UnicodeString(newLoc.getName())  +" oldLoc= "+UnicodeString(oldLoc.getName()));
        }
        logln("Calendar(getLocale) old:"+UnicodeString(l1)+"   new:"+UnicodeString(l2));   
        delete c1;
        delete c2;
    }
#endif
}
void LocaleAliasTest::TestDateFormat() {
#if !UCONFIG_NO_FORMATTING
    UErrorCode status = U_ZERO_ERROR;
    for (int i=0; i<_LOCALE_NUMBER; i++) {
        Locale oldLoc(_LOCALE_ALIAS[i][0]);
        Locale newLoc(_LOCALE_ALIAS[i][1]);
        if(!isLocaleAvailable(_LOCALE_ALIAS[i][1])){
            logln(UnicodeString(newLoc.getName())+" is not available. Skipping!");
            continue;
        }
        logln("\nold locale:%s   new locale:%s",oldLoc.getName(),newLoc.getName());

        DateFormat* df1 = DateFormat::createDateInstance(DateFormat::FULL, oldLoc);
        DateFormat* df2 = DateFormat::createDateInstance(DateFormat::FULL, newLoc);

        //Test function "getLocale"
        const char* l1 = df1->getLocaleID(ULOC_VALID_LOCALE, status);
        const char* l2 = df2->getLocaleID(ULOC_VALID_LOCALE, status);
        if (strcmp(newLoc.getName(), l1)!=0) {
            errln("CalendarTest: newLoc!=l1: newLoc= "+UnicodeString(newLoc.getName()) +" l1= "+UnicodeString(l1));
        }
        if (strcmp(l1, l2)!=0) {
            errln("TestDateFormat: l1!=l2: l1= "+UnicodeString(l1) +" l2= "+UnicodeString(l2));
        }
        if(!(df1==df2)){
            errln("TestDateFormat: c1!=c2.  newLoc= "+UnicodeString(newLoc.getName())  +" oldLoc= "+UnicodeString(oldLoc.getName()));
        }
        logln("DateFormat(getLocale) old:%s   new:%s",l1,l2);

        delete df1;
        delete df2;
    }
#endif
}
void LocaleAliasTest::TestCollation() {
#if !UCONFIG_NO_COLLATION
    UErrorCode status = U_ZERO_ERROR;
    for (int i=0; i<_LOCALE_NUMBER; i++) {
        Locale oldLoc(_LOCALE_ALIAS[i][0]);
        Locale newLoc(_LOCALE_ALIAS[i][1]);
        if(!isLocaleAvailable(_LOCALE_ALIAS[i][1])){
            logln(UnicodeString(newLoc.getName())+" is not available. Skipping!");
            continue;
        }
        logln("\nold locale:%s   new locale:%s",oldLoc.getName(),newLoc.getName());

        Collator* c1 = Collator::createInstance(oldLoc, status);
        Collator* c2 = Collator::createInstance(newLoc, status);

        Locale l1 = c1->getLocale(ULOC_VALID_LOCALE, status);
        Locale l2 = c2->getLocale(ULOC_VALID_LOCALE, status);

        if (strcmp(newLoc.getName(), l1.getName())!=0) {
            errln("CalendarTest: newLoc!=l1: newLoc= "+UnicodeString(newLoc.getName()) +" l1= "+UnicodeString(l1.getName()));
        }
        if (strcmp(l1.getName(), l2.getName())!=0) {
            errln("CollationTest: l1!=l2: l1= "+UnicodeString(l1.getName()) +" l2= "+UnicodeString(l2.getName()));
        }
        if(!(c1==c2)){
            errln("CollationTest: c1!=c2.  newLoc= "+UnicodeString(newLoc.getName())  +" oldLoc= "+UnicodeString(oldLoc.getName()));
        }
        logln("Collator(getLocale) old:%s   new:%s", l1.getName(), l2.getName());
        delete c1;
        delete c2;
    }
#endif
}
void LocaleAliasTest::TestULocale() {
    for (int i=0; i<_LOCALE_NUMBER; i++) {
        Locale oldLoc(_LOCALE_ALIAS[i][0]);
        Locale newLoc(_LOCALE_ALIAS[i][1]);
        if(!isLocaleAvailable(_LOCALE_ALIAS[i][1])){
            logln(UnicodeString(newLoc.getName())+" is not available. Skipping!");
            continue;
        }
        logln("\nold locale:%s   new locale:%s",oldLoc.getName(),newLoc.getName());

        UnicodeString name1, name2;
        oldLoc.getDisplayName(name1);
        newLoc.getDisplayName(name2);
        if (name1!=name2) {
            errln("DisplayNames are not equal.  newLoc= "+UnicodeString(newLoc.getName())  +" oldLoc= "+UnicodeString(oldLoc.getName()));
        }
        log("ULocale(getDisplayName) old:");
        log(name1);
        log("   new:");
        logln(name2);
    }
}
LocaleAliasTest::LocaleAliasTest(){
    UErrorCode status = U_ZERO_ERROR;
    resIndex = ures_open(NULL,"res_index", &status);
    if(U_FAILURE(status)){
        errln("Could not open res_index.res. Exiting. Error: %s\n", u_errorName(status));
        resIndex=NULL;
    }
    defLocale = Locale::getDefault();
    Locale::setDefault(Locale::getUS(), status); 
}
LocaleAliasTest::~LocaleAliasTest(){
    /* reset the default locale */
    UErrorCode status = U_ZERO_ERROR;
    Locale::setDefault(defLocale, status); 
    ures_close(resIndex);
    if(U_FAILURE(status)){
        errln("Could not reset the default locale. Exiting. Error: %s\n", u_errorName(status));
    }
}
UBool LocaleAliasTest::isLocaleAvailable(const char* loc){
    if(resIndex==NULL){
        return FALSE;
    }
    UErrorCode status = U_ZERO_ERROR;
    int32_t len = 0;
    ures_getStringByKey(resIndex, loc,&len, &status);
    if(U_FAILURE(status)){
        return FALSE; 
    }
    return TRUE;
}
void LocaleAliasTest::TestDisplayName() {
    int32_t availableNum =0;
    const Locale* available = Locale::getAvailableLocales(availableNum);
    for (int i=0; i<_LOCALE_NUMBER; i++) {
        Locale oldLoc(_LOCALE_ALIAS[i][0]);
        Locale newLoc(_LOCALE_ALIAS[i][1]);
        if(!isLocaleAvailable(_LOCALE_ALIAS[i][1])){
            logln(UnicodeString(newLoc.getName())+" is not available. Skipping!");
            continue;
        }
        for(int j=0; j<availableNum; j++){
            UnicodeString dipLocName = UnicodeString(available[j].getName());
            const UnicodeString oldCountry = oldLoc.getDisplayCountry(dipLocName);
            const UnicodeString newCountry = newLoc.getDisplayCountry(dipLocName);
            const UnicodeString oldLang = oldLoc.getDisplayLanguage(dipLocName);
            const UnicodeString newLang = newLoc.getDisplayLanguage(dipLocName);

            // is  there  display name for the current country ID               
            if(newCountry != newLoc.getCountry()){
                if(oldCountry!=newCountry){
                    errln("getCountry() failed for "+ UnicodeString(oldLoc.getName()) +" oldCountry= "+ prettify(oldCountry) +" newCountry = "+prettify(newCountry)+ " in display locale "+ UnicodeString(available[j].getName()));
                }
            }
            //there is a display name for the current lang ID               
            if(newLang != newLoc.getLanguage()){
                if(oldLang != newLang){
                    errln("getLanguage() failed for " + UnicodeString(oldLoc.getName()) + " oldLang = "+ prettify(oldLang) +" newLang = "+prettify(newLang)+ " in display locale "+UnicodeString(available[j].getName()));
                }
            }
        }
    }
}
void LocaleAliasTest::TestUResourceBundle() {

    UErrorCode status = U_ZERO_ERROR;
    for (int i=0; i<_LOCALE_NUMBER; i++) {
        Locale oldLoc(_LOCALE_ALIAS[i][0]);
        Locale newLoc(_LOCALE_ALIAS[i][1]);
        if(!isLocaleAvailable(_LOCALE_ALIAS[i][1])){
            logln(UnicodeString(newLoc.getName())+" is not available. Skipping!");
            continue;
        }
        logln("\nold locale:%s   new locale:%s",oldLoc.getName(),newLoc.getName());

        ResourceBundle* rb1 = NULL;
        ResourceBundle* rb2 = NULL;

        const char* testdatapath=loadTestData(status);

        UnicodeString us1("NULL");
        UnicodeString us2("NULL");
        rb1 = new ResourceBundle(testdatapath, oldLoc, status);
        if (U_FAILURE(U_ZERO_ERROR)) {

        } else {
            us1 = rb1->getStringEx("locale", status);
        }
        rb2 = new ResourceBundle(testdatapath, newLoc, status);
        if (U_FAILURE(U_ZERO_ERROR)){

        } else {
            us2 = rb2->getStringEx("locale", status);
        }
        UnicodeString uNewLoc(newLoc.getName());
        if (us1.compare(uNewLoc)!=0 || us1.compare(us2)!=0 || status!=U_ZERO_ERROR) {

        }
        log("UResourceBundle(getStringEx) old:");
        log(us1);
        log("   new:");
        logln(us2);

        if (rb1!=NULL) {
            delete rb1;
        }
        if (rb2!=NULL) {
            delete rb2;
        }
    }

}
