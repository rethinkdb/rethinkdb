/**
 *******************************************************************************
 * Copyright (C) 2001-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 *******************************************************************************
 */

#include "unicode/utypeinfo.h"  // for 'typeid' to work

#include "unicode/utypes.h"

#if !UCONFIG_NO_SERVICE

#include "icusvtst.h"
#include "servloc.h"
#include <stdio.h>


class MyListener : public EventListener {
};

class WrongListener : public EventListener {
};

class ICUNSubclass : public ICUNotifier {
    public:
    UBool acceptsListener(const EventListener& /*l*/) const {
        return TRUE;
        // return l instanceof MyListener;
    }

    virtual void notifyListener(EventListener& /*l*/) const {
    }
};

// This factory does nothing
class LKFSubclass0 : public LocaleKeyFactory {
public:
        LKFSubclass0()
                : LocaleKeyFactory(VISIBLE, "LKFSubclass0")
        {
        }
};

class LKFSubclass : public LocaleKeyFactory {
    Hashtable table;

    public:
    LKFSubclass(UBool visible) 
        : LocaleKeyFactory(visible ? VISIBLE : INVISIBLE, "LKFSubclass")
    {
        UErrorCode status = U_ZERO_ERROR;
        table.put("en_US", this, status);
    }

    protected:
    virtual const Hashtable* getSupportedIDs(UErrorCode &/*status*/) const {
        return &table;
    }
};

class Integer : public UObject {
    public:
    const int32_t _val;

    Integer(int32_t val) : _val(val) {
    }

    Integer(const Integer& rhs) : UObject(rhs), _val(rhs._val) {
    }
    virtual ~Integer() {
    }

    public:
    /**
     * UObject boilerplate.
     */
    static UClassID getStaticClassID() { 
        return (UClassID)&fgClassID;
    }

    virtual UClassID getDynamicClassID() const {
        return getStaticClassID();
    }

    virtual UBool operator==(const UObject& other) const 
    {
        return typeid(*this) == typeid(other) &&
            _val == ((Integer&)other)._val;
    }

    public:
    virtual UnicodeString& debug(UnicodeString& result) const {
        debugClass(result);
        result.append(" val: ");
        result.append(_val);
        return result;
    }

    virtual UnicodeString& debugClass(UnicodeString& result) const {
        return result.append("Integer");
    }

    private:
    static const char fgClassID;
};

const char Integer::fgClassID = '\0';

// use locale keys
class TestIntegerService : public ICUService {
    public:
    ICUServiceKey* createKey(const UnicodeString* id, UErrorCode& status) const {
        return LocaleKey::createWithCanonicalFallback(id, NULL, status); // no fallback locale
    }

    virtual ICUServiceFactory* createSimpleFactory(UObject* obj, const UnicodeString& id, UBool visible, UErrorCode& status) 
    {
        Integer* i;
        if (U_SUCCESS(status) && obj && (i = dynamic_cast<Integer*>(obj)) != NULL) {
            return new SimpleFactory(i, id, visible);
        }
        return NULL;
    }

    virtual UObject* cloneInstance(UObject* instance) const {
        return instance ? new Integer(*(Integer*)instance) : NULL;
    }
};


ICUServiceTest::ICUServiceTest() {
}

ICUServiceTest::~ICUServiceTest() {
}

void 
ICUServiceTest::runIndexedTest(int32_t index, UBool exec, const char* &name,
char* /*par*/)
{
    switch (index) {
        TESTCASE(0,testAPI_One);
        TESTCASE(1,testAPI_Two);
        TESTCASE(2,testRBF);
        TESTCASE(3,testNotification);
        TESTCASE(4,testLocale);
        TESTCASE(5,testWrapFactory);
        TESTCASE(6,testCoverage);
    default: name = ""; break;
    }
}

UnicodeString append(UnicodeString& result, const UObject* obj) 
{
    char buffer[128];
    if (obj == NULL) {
        result.append("NULL");
    } else {
        const UnicodeString* s;
        const Locale* loc;
        const Integer* i;
        if ((s = dynamic_cast<const UnicodeString*>(obj)) != NULL) {
            result.append(*s);
        } else if ((loc = dynamic_cast<const Locale*>(obj)) != NULL) {
            result.append(loc->getName());
        } else if ((i = dynamic_cast<const Integer*>(obj)) != NULL) {
            sprintf(buffer, "%d", (int)i->_val);
            result.append(buffer);
        } else {
            sprintf(buffer, "%p", (const void*)obj);
            result.append(buffer);
        }
    }
    return result;
}

UnicodeString& 
ICUServiceTest::lrmsg(UnicodeString& result, const UnicodeString& message, const UObject* lhs, const UObject* rhs) const 
{
    result.append(message);
    result.append(" lhs: ");
    append(result, lhs);
    result.append(", rhs: ");
    append(result, rhs);
    return result;
}

void 
ICUServiceTest::confirmBoolean(const UnicodeString& message, UBool val)
{
    if (val) {
        logln(message);
    } else {
        errln(message);
    }
}

#if 0
void
ICUServiceTest::confirmEqual(const UnicodeString& message, const UObject* lhs, const UObject* rhs) 
{
    UBool equ = (lhs == NULL)
        ? (rhs == NULL)
        : (rhs != NULL && lhs->operator==(*rhs));

    UnicodeString temp;
    lrmsg(temp, message, lhs, rhs);

    if (equ) {
        logln(temp);
    } else {
        errln(temp);
    }
}
#else
void
ICUServiceTest::confirmEqual(const UnicodeString& message, const Integer* lhs, const Integer* rhs) 
{
    UBool equ = (lhs == NULL)
        ? (rhs == NULL)
        : (rhs != NULL && lhs->operator==(*rhs));

    UnicodeString temp;
    lrmsg(temp, message, lhs, rhs);

    if (equ) {
        logln(temp);
    } else {
        errln(temp);
    }
}

void
ICUServiceTest::confirmEqual(const UnicodeString& message, const UnicodeString* lhs, const UnicodeString* rhs) 
{
    UBool equ = (lhs == NULL)
        ? (rhs == NULL)
        : (rhs != NULL && lhs->operator==(*rhs));

    UnicodeString temp;
    lrmsg(temp, message, lhs, rhs);

    if (equ) {
        logln(temp);
    } else {
        errln(temp);
    }
}

void
ICUServiceTest::confirmEqual(const UnicodeString& message, const Locale* lhs, const Locale* rhs) 
{
    UBool equ = (lhs == NULL)
        ? (rhs == NULL)
        : (rhs != NULL && lhs->operator==(*rhs));

    UnicodeString temp;
    lrmsg(temp, message, lhs, rhs);

    if (equ) {
        logln(temp);
    } else {
        errln(temp);
    }
}
#endif

// use these for now
void
ICUServiceTest::confirmStringsEqual(const UnicodeString& message, const UnicodeString& lhs, const UnicodeString& rhs) 
{
    UBool equ = lhs == rhs;

    UnicodeString temp = message;
    temp.append(" lhs: ");
    temp.append(lhs);
    temp.append(" rhs: ");
    temp.append(rhs);

    if (equ) {
        logln(temp);
    } else {
        dataerrln(temp);
    }
}


void 
ICUServiceTest::confirmIdentical(const UnicodeString& message, const UObject* lhs, const UObject *rhs) 
{
    UnicodeString temp;
    lrmsg(temp, message, lhs, rhs);
    if (lhs == rhs) {
        logln(temp);
    } else {
        errln(temp);
    }
}

void
ICUServiceTest::confirmIdentical(const UnicodeString& message, int32_t lhs, int32_t rhs)  
{
    if (lhs == rhs) {
        logln(message + " lhs: " + lhs + " rhs: " + rhs);
    } else {
        errln(message + " lhs: " + lhs + " rhs: " + rhs);
    }
}

void
ICUServiceTest::msgstr(const UnicodeString& message, UObject* obj, UBool err)
{
    if (obj) {
    UnicodeString* str = (UnicodeString*)obj;
        logln(message + *str);
        delete str;
    } else if (err) {
        errln("Error " + message + "string is NULL");
    }
}

void 
ICUServiceTest::testAPI_One() 
{
    // create a service using locale keys,
    TestIntegerService service;

    // register an object with one locale, 
    // search for an object with a more specific locale
    // should return the original object
    UErrorCode status = U_ZERO_ERROR;
    Integer* singleton0 = new Integer(0);
    service.registerInstance(singleton0, "en_US", status);
    {
        UErrorCode status = U_ZERO_ERROR;
        Integer* result = (Integer*)service.get("en_US_FOO", status);
        confirmEqual("1) en_US_FOO -> en_US", result, singleton0);
        delete result;
    }

    // register a new object with the more specific locale
    // search for an object with that locale
    // should return the new object
    Integer* singleton1 = new Integer(1);
    service.registerInstance(singleton1, "en_US_FOO", status);
    {
        UErrorCode status = U_ZERO_ERROR;
        Integer* result = (Integer*)service.get("en_US_FOO", status);
        confirmEqual("2) en_US_FOO -> en_US_FOO", result, singleton1);
        delete result;
    }

    // search for an object that falls back to the first registered locale
    {
        UErrorCode status = U_ZERO_ERROR;
        Integer* result = (Integer*)service.get("en_US_BAR", status);
        confirmEqual("3) en_US_BAR -> en_US", result, singleton0);
        delete result;
    }

    // get a list of the factories, should be two
    {
        confirmIdentical("4) factory size", service.countFactories(), 2);
    }

    // register a new object with yet another locale
    Integer* singleton2 = new Integer(2);
    service.registerInstance(singleton2, "en", status);
    {
        confirmIdentical("5) factory size", service.countFactories(), 3);
    }

    // search for an object with the new locale
    // stack of factories is now en, en_US_FOO, en_US
    // search for en_US should still find en_US object
    {
        UErrorCode status = U_ZERO_ERROR;
        Integer* result = (Integer*)service.get("en_US_BAR", status);
        confirmEqual("6) en_US_BAR -> en_US", result, singleton0);
        delete result;
    }

    // register a new object with an old id, should hide earlier factory using this id, but leave it there
    Integer* singleton3 = new Integer(3);
    URegistryKey s3key = service.registerInstance(singleton3, "en_US", status);
    {
        confirmIdentical("9) factory size", service.countFactories(), 4);
    }

    // should get data from that new factory
    {
        UErrorCode status = U_ZERO_ERROR;
        Integer* result = (Integer*)service.get("en_US_BAR", status);
        confirmEqual("10) en_US_BAR -> (3)", result, singleton3);
        delete result;
    }

    // remove new factory
    // should have fewer factories again
    // singleton3 dead!
    {
        UErrorCode status = U_ZERO_ERROR;
        service.unregister(s3key, status);
        confirmIdentical("11) factory size", service.countFactories(), 3);
    }

    // should get original data again after remove factory
    {
        UErrorCode status = U_ZERO_ERROR;
        Integer* result = (Integer*)service.get("en_US_BAR", status);
        confirmEqual("12) en_US_BAR -> (3)", result, singleton0);
        delete result;
    }

    // shouldn't find unregistered ids
    {
        UErrorCode status = U_ZERO_ERROR;
        Integer* result = (Integer*)service.get("foo", status);
        confirmIdentical("13) foo -> null", result, NULL);
        delete result;
    }

    // should find non-canonical strings
    {
        UnicodeString resultID;
        UErrorCode status = U_ZERO_ERROR;
        Integer* result = (Integer*)service.get("EN_us_fOo", &resultID, status);
        confirmEqual("14a) find-non-canonical", result, singleton1);
        confirmStringsEqual("14b) find non-canonical", resultID, "en_US_FOO");
        delete result;
    }

    // should be able to register non-canonical strings and get them canonicalized
    Integer* singleton4 = new Integer(4);
    service.registerInstance(singleton4, "eN_ca_dUde", status);
    {
        UnicodeString resultID;
        UErrorCode status = U_ZERO_ERROR;
        Integer* result = (Integer*)service.get("En_Ca_DuDe", &resultID, status);
        confirmEqual("15a) find-non-canonical", result, singleton4);
        confirmStringsEqual("15b) register non-canonical", resultID, "en_CA_DUDE");
        delete result;
    }

    // should be able to register invisible factories, these will not
    // be visible by default, but if you know the secret password you
    // can still access these services...
    Integer* singleton5 = new Integer(5);
    service.registerInstance(singleton5, "en_US_BAR", FALSE, status);
    {
        UErrorCode status = U_ZERO_ERROR;
        Integer* result = (Integer*)service.get("en_US_BAR", status);
        confirmEqual("17) get invisible", result, singleton5);
        delete result;
    }
    
    // should not be able to locate invisible services
    {
        UErrorCode status = U_ZERO_ERROR;
        UVector ids(uhash_deleteUnicodeString, uhash_compareUnicodeString, status);
        service.getVisibleIDs(ids, status);
        UnicodeString target = "en_US_BAR";
        confirmBoolean("18) find invisible", !ids.contains(&target));
    }

    // clear factory and caches
    service.reset();
    confirmBoolean("19) is default", service.isDefault());
}

/*
 ******************************************************************
 */
class TestStringSimpleKeyService : public ICUService {
public:
 
        virtual ICUServiceFactory* createSimpleFactory(UObject* obj, const UnicodeString& id, UBool visible, UErrorCode& status) 
    {
                // We could put this type check into ICUService itself, but we'd still
                // have to implement cloneInstance.  Otherwise we could just tell the service
                // what the object type is when we create it, and the default implementation
                // could handle everything for us.  Phooey.
        if (obj && dynamic_cast<UnicodeString*>(obj) != NULL) {
                        return ICUService::createSimpleFactory(obj, id, visible, status);
        }
        return NULL;
    }

    virtual UObject* cloneInstance(UObject* instance) const {
        return instance ? new UnicodeString(*(UnicodeString*)instance) : NULL;
    }
};

class TestStringService : public ICUService {
    public:
    ICUServiceKey* createKey(const UnicodeString* id, UErrorCode& status) const {
        return LocaleKey::createWithCanonicalFallback(id, NULL, status); // no fallback locale
    }

  virtual ICUServiceFactory* createSimpleFactory(UObject* obj, const UnicodeString& id, UBool visible, UErrorCode& /* status */) 
    {
        UnicodeString* s;
        if (obj && (s = dynamic_cast<UnicodeString*>(obj)) != NULL) {
            return new SimpleFactory(s, id, visible);
        }
        return NULL;
    }

    virtual UObject* cloneInstance(UObject* instance) const {
        return instance ? new UnicodeString(*(UnicodeString*)instance) : NULL;
    }
};

// this creates a string for any id, but doesn't report anything
class AnonymousStringFactory : public ICUServiceFactory
{
    public:
    virtual UObject* create(const ICUServiceKey& key, const ICUService* /* service */, UErrorCode& /* status */) const {
        return new UnicodeString(key.getID());
    }

    virtual void updateVisibleIDs(Hashtable& /*result*/, UErrorCode& /*status*/) const {
        // do nothing
    }

    virtual UnicodeString& getDisplayName(const UnicodeString& /*id*/, const Locale& /*locale*/, UnicodeString& result) const {
        // do nothing
        return result;
    }

    static UClassID getStaticClassID() {
        return (UClassID)&fgClassID;
    }

    virtual UClassID getDynamicClassID() const {
        return getStaticClassID();
    }

    private:
    static const char fgClassID;
};

const char AnonymousStringFactory::fgClassID = '\0';

class TestMultipleKeyStringFactory : public ICUServiceFactory {
    UErrorCode _status;
    UVector _ids;
    UnicodeString _factoryID;

    public:
    TestMultipleKeyStringFactory(const UnicodeString ids[], int32_t count, const UnicodeString& factoryID)
        : _status(U_ZERO_ERROR)
        , _ids(uhash_deleteUnicodeString, uhash_compareUnicodeString, count, _status)
        , _factoryID(factoryID + ": ") 
    {
        for (int i = 0; i < count; ++i) {
            _ids.addElement(new UnicodeString(ids[i]), _status);
        }
    }
  
    ~TestMultipleKeyStringFactory() {
    }

    UObject* create(const ICUServiceKey& key, const ICUService* /* service */, UErrorCode& status) const {
        if (U_FAILURE(status)) {
        return NULL;
        }
        UnicodeString temp;
        key.currentID(temp);
        if (U_SUCCESS(_status)) {
        if (_ids.contains(&temp)) {
                return new UnicodeString(_factoryID + temp);
        }
        } else {
        status = _status;
    }
        return NULL;
    }

    void updateVisibleIDs(Hashtable& result, UErrorCode& status) const {
        if (U_SUCCESS(_status)) {
            for (int32_t i = 0; i < _ids.size(); ++i) {
                result.put(*(UnicodeString*)_ids[i], (void*)this, status);
            }
        }
    }

    UnicodeString& getDisplayName(const UnicodeString& id, const Locale& locale, UnicodeString& result) const {
        if (U_SUCCESS(_status) && _ids.contains((void*)&id)) {
            char buffer[128];
            UErrorCode status = U_ZERO_ERROR;
            int32_t len = id.extract(buffer, sizeof(buffer), NULL, status);
            if (U_SUCCESS(status)) {
                if (len == sizeof(buffer)) {
                    --len;
                }
                buffer[len] = 0;
                Locale loc = Locale::createFromName(buffer);
                loc.getDisplayName(locale, result);
                return result;
            }
        }
        result.setToBogus(); // shouldn't happen
        return result;
    }

    static UClassID getStaticClassID() {
        return (UClassID)&fgClassID;
    }

    virtual UClassID getDynamicClassID() const {
        return getStaticClassID();
    }

    private:
    static const char fgClassID;
};

const char TestMultipleKeyStringFactory::fgClassID = '\0';

void 
ICUServiceTest::testAPI_Two()
{
    UErrorCode status = U_ZERO_ERROR;
    TestStringService service;
    service.registerFactory(new AnonymousStringFactory(), status);

    // anonymous factory will still handle the id
    {
        UErrorCode status = U_ZERO_ERROR;
        const UnicodeString en_US = "en_US";
        UnicodeString* result = (UnicodeString*)service.get(en_US, status);
        confirmEqual("21) locale", result, &en_US);
        delete result;
    }

    // still normalizes id
    {
        UErrorCode status = U_ZERO_ERROR;
        const UnicodeString en_US_BAR = "en_US_BAR";
        UnicodeString resultID;
        UnicodeString* result = (UnicodeString*)service.get("EN_us_bar", &resultID, status);
        confirmEqual("22) locale", &resultID, &en_US_BAR);
        delete result;
    }

    // we can override for particular ids
    UnicodeString* singleton0 = new UnicodeString("Zero");
    service.registerInstance(singleton0, "en_US_BAR", status);
    {
        UErrorCode status = U_ZERO_ERROR;
        UnicodeString* result = (UnicodeString*)service.get("en_US_BAR", status);
        confirmEqual("23) override super", result, singleton0);
        delete result;
    }

    // empty service should not recognize anything 
    service.reset();
    {
        UErrorCode status = U_ZERO_ERROR;
        UnicodeString* result = (UnicodeString*)service.get("en_US", status);
        confirmIdentical("24) empty", result, NULL);
    }

    // create a custom multiple key factory
    {
        UnicodeString xids[] = {
            "en_US_VALLEY_GIRL", 
            "en_US_VALLEY_BOY",
            "en_US_SURFER_GAL",
            "en_US_SURFER_DUDE"
        };
        int32_t count = sizeof(xids)/sizeof(UnicodeString);

        ICUServiceFactory* f = new TestMultipleKeyStringFactory(xids, count, "Later");
        service.registerFactory(f, status);
    }

    // iterate over the visual ids returned by the multiple factory
    {
        UErrorCode status = U_ZERO_ERROR;
        UVector ids(uhash_deleteUnicodeString, uhash_compareUnicodeString, 0, status);
        service.getVisibleIDs(ids, status);
        for (int i = 0; i < ids.size(); ++i) {
            const UnicodeString* id = (const UnicodeString*)ids[i];
            UnicodeString* result = (UnicodeString*)service.get(*id, status);
            if (result) {
                logln("  " + *id + " --> " + *result);
                delete result;
            } else {
                errln("could not find " + *id);
            }
        }
        // four visible ids
        confirmIdentical("25) visible ids", ids.size(), 4);
    }

    // iterate over the display names
    {
        UErrorCode status = U_ZERO_ERROR;
        UVector names(status);
        service.getDisplayNames(names, status);
        for (int i = 0; i < names.size(); ++i) {
            const StringPair* pair = (const StringPair*)names[i];
            logln("  " + pair->displayName + " --> " + pair->id);
        }
        confirmIdentical("26) display names", names.size(), 4);
    }

    // no valid display name
    {
        UnicodeString name;
        service.getDisplayName("en_US_VALLEY_GEEK", name);
        confirmBoolean("27) get display name", name.isBogus());
    }

    {
        UnicodeString name;
        service.getDisplayName("en_US_SURFER_DUDE", name, Locale::getEnglish());
        confirmStringsEqual("28) get display name", name, "English (United States, SURFER_DUDE)");
    }

    // register another multiple factory
    {
        UnicodeString xids[] = {
            "en_US_SURFER", 
            "en_US_SURFER_GAL", 
            "en_US_SILICON", 
            "en_US_SILICON_GEEK",
        };
        int32_t count = sizeof(xids)/sizeof(UnicodeString);

        ICUServiceFactory* f = new TestMultipleKeyStringFactory(xids, count, "Rad dude");
        service.registerFactory(f, status);
    }

    // this time, we have seven display names
    // Rad dude's surfer gal 'replaces' Later's surfer gal
    {
        UErrorCode status = U_ZERO_ERROR;
        UVector names(status);
        service.getDisplayNames(names, Locale("es"), status);
        for (int i = 0; i < names.size(); ++i) {
            const StringPair* pair = (const StringPair*)names[i];
            logln("  " + pair->displayName + " --> " + pair->id);
        }
        confirmIdentical("29) display names", names.size(), 7);
    }

    // we should get the display name corresponding to the actual id
    // returned by the id we used.
    {
        UErrorCode status = U_ZERO_ERROR;
        UnicodeString actualID;
        UnicodeString id = "en_us_surfer_gal";
        UnicodeString* gal = (UnicodeString*)service.get(id, &actualID, status);
        if (gal != NULL) {
            UnicodeString displayName;
            logln("actual id: " + actualID);
            service.getDisplayName(actualID, displayName, Locale::getEnglish());
            logln("found actual: " + *gal + " with display name: " + displayName);
            confirmBoolean("30) found display name for actual", !displayName.isBogus());

            service.getDisplayName(id, displayName, Locale::getEnglish());
            logln("found actual: " + *gal + " with display name: " + displayName);
            confirmBoolean("31) found display name for query", displayName.isBogus());

            delete gal;
        } else {
            errln("30) service could not find entry for " + id);
        }
    }

    // this should be handled by the 'dude' factory, since it overrides en_US_SURFER.
    {
        UErrorCode status = U_ZERO_ERROR;
        UnicodeString actualID;
        UnicodeString id = "en_US_SURFER_BOZO";
        UnicodeString* bozo = (UnicodeString*)service.get(id, &actualID, status);
        if (bozo != NULL) {
            UnicodeString displayName;
            service.getDisplayName(actualID, displayName, Locale::getEnglish());
            logln("found actual: " + *bozo + " with display name: " + displayName);
            confirmBoolean("32) found display name for actual", !displayName.isBogus());

            service.getDisplayName(id, displayName, Locale::getEnglish());
            logln("found actual: " + *bozo + " with display name: " + displayName);
            confirmBoolean("33) found display name for query", displayName.isBogus());

            delete bozo;
        } else {
            errln("32) service could not find entry for " + id);
        }
    }

    // certainly not default...
    {
        confirmBoolean("34) is default ", !service.isDefault());
    }

    {
        UErrorCode status = U_ZERO_ERROR;
        UVector ids(uhash_deleteUnicodeString, uhash_compareUnicodeString, 0, status);
        service.getVisibleIDs(ids, status);
        for (int i = 0; i < ids.size(); ++i) {
            const UnicodeString* id = (const UnicodeString*)ids[i];
            msgstr(*id + "? ", service.get(*id, status));
        }

        logstr("valleygirl?  ", service.get("en_US_VALLEY_GIRL", status));
        logstr("valleyboy?   ", service.get("en_US_VALLEY_BOY", status));
        logstr("valleydude?  ", service.get("en_US_VALLEY_DUDE", status));
        logstr("surfergirl?  ", service.get("en_US_SURFER_GIRL", status));
    }
}


class CalifornioLanguageFactory : public ICUResourceBundleFactory 
{
    public:
    static const char* californio; // = "en_US_CA";
    static const char* valley; // = californio ## "_VALLEY";
    static const char* surfer; // = californio ## "_SURFER";
    static const char* geek; // = californio ## "_GEEK";
    static Hashtable* supportedIDs; // = NULL;

    static void cleanup(void) {
      delete supportedIDs;
      supportedIDs = NULL;
    }

    const Hashtable* getSupportedIDs(UErrorCode& status) const 
    {
        if (supportedIDs == NULL) {
            Hashtable* table = new Hashtable();
            table->put(UnicodeString(californio), (void*)table, status);
            table->put(UnicodeString(valley), (void*)table, status);
            table->put(UnicodeString(surfer), (void*)table, status);
            table->put(UnicodeString(geek), (void*)table, status);

            // not necessarily atomic, but this is a test...
            supportedIDs = table;
        }
        return supportedIDs;
    }

    UnicodeString& getDisplayName(const UnicodeString& id, const Locale& locale, UnicodeString& result) const 
    {
        UnicodeString prefix = "";
        UnicodeString suffix = "";
        UnicodeString ls = locale.getName();
        if (LocaleUtility::isFallbackOf(californio, ls)) {
            if (!ls.caseCompare(valley, 0)) {
                prefix = "Like, you know, it's so totally ";
            } else if (!ls.caseCompare(surfer, 0)) {
                prefix = "Dude, it's ";
            } else if (!ls.caseCompare(geek, 0)) {
                prefix = "I'd estimate it is approximately ";
            } else {
                prefix = "Huh?  Maybe ";
            }
        }
        if (LocaleUtility::isFallbackOf(californio, id)) {
            if (!id.caseCompare(valley, 0)) {
                suffix = "like the Valley, you know?  Let's go to the mall!";
            } else if (!id.caseCompare(surfer, 0)) {
                suffix = "time to hit those gnarly waves, Dude!!!";
            } else if (!id.caseCompare(geek, 0)) {
                suffix = "all systems go.  T-Minus 9, 8, 7...";
            } else {
                suffix = "No Habla Englais";
            }
        } else {
            suffix = ICUResourceBundleFactory::getDisplayName(id, locale, result);
        }

        result = prefix + suffix;
        return result;
    }
};

const char* CalifornioLanguageFactory::californio = "en_US_CA";
const char* CalifornioLanguageFactory::valley = "en_US_CA_VALLEY";
const char* CalifornioLanguageFactory::surfer = "en_US_CA_SURFER";
const char* CalifornioLanguageFactory::geek = "en_US_CA_GEEK";
Hashtable* CalifornioLanguageFactory::supportedIDs = NULL;

void
ICUServiceTest::testRBF()
{
    // resource bundle factory.
    UErrorCode status = U_ZERO_ERROR;
    TestStringService service;
    service.registerFactory(new ICUResourceBundleFactory(), status);

    // list all of the resources 
    {
        UErrorCode status = U_ZERO_ERROR;
        UVector ids(uhash_deleteUnicodeString, uhash_compareUnicodeString, 0, status);
        service.getVisibleIDs(ids, status);
        logln("all visible ids:");
        for (int i = 0; i < ids.size(); ++i) {
            const UnicodeString* id = (const UnicodeString*)ids[i];
            logln(*id);
        }
    }

    // get all the display names of these resources
    // this should be fast since the display names were cached.
    {
        UErrorCode status = U_ZERO_ERROR;
        UVector names(status);
        service.getDisplayNames(names, Locale::getGermany(), status);
        logln("service display names for de_DE");
        for (int i = 0; i < names.size(); ++i) {
            const StringPair* pair = (const StringPair*)names[i];
            logln("  " + pair->displayName + " --> " + pair->id);
        }
    }

    service.registerFactory(new CalifornioLanguageFactory(), status);

    // get all the display names of these resources
    {
        logln("californio language factory:");
        const char* idNames[] = {
            CalifornioLanguageFactory::californio, 
            CalifornioLanguageFactory::valley, 
            CalifornioLanguageFactory::surfer, 
            CalifornioLanguageFactory::geek,
        };
        int32_t count = sizeof(idNames)/sizeof(idNames[0]);

        for (int i = 0; i < count; ++i) {
            logln(UnicodeString("\n  --- ") + idNames[i] + " ---");
            {
                UErrorCode status = U_ZERO_ERROR;
                UVector names(status);
                service.getDisplayNames(names, idNames[i], status);
                for (int i = 0; i < names.size(); ++i) {
                    const StringPair* pair = (const StringPair*)names[i];
                    logln("  " + pair->displayName + " --> " + pair->id);
                }
            }
        }
    }
    CalifornioLanguageFactory::cleanup();
}

class SimpleListener : public ServiceListener {
    ICUServiceTest* _test;
    int32_t _n;
    UnicodeString _name;

    public:
    SimpleListener(ICUServiceTest* test, const UnicodeString& name) : _test(test), _n(0), _name(name) {}

    virtual void serviceChanged(const ICUService& service) const {
        UnicodeString serviceName = "listener ";
        serviceName.append(_name);
        serviceName.append(" n++");
        serviceName.append(" service changed: " );
        service.getName(serviceName);
        _test->logln(serviceName);
    }
};

void
ICUServiceTest::testNotification()
{
    SimpleListener one(this, "one");
    SimpleListener two(this, "two");
    {
        UErrorCode status = U_ZERO_ERROR;

        logln("simple registration notification");
        TestStringService ls;
        ls.addListener(&one, status);
        ls.addListener(&two, status);

        logln("registering foo... ");
        ls.registerInstance(new UnicodeString("Foo"), "en_FOO", status);
        logln("registering bar... ");
        ls.registerInstance(new UnicodeString("Bar"), "en_BAR", status);
        logln("getting foo...");
        UnicodeString* result = (UnicodeString*)ls.get("en_FOO", status);
        logln(*result);
        delete result;

        logln("removing listener 2...");
        ls.removeListener(&two, status);
        logln("registering baz...");
        ls.registerInstance(new UnicodeString("Baz"), "en_BAZ", status);
        logln("removing listener 1");
        ls.removeListener(&one, status);
        logln("registering burp...");
        ls.registerInstance(new UnicodeString("Burp"), "en_BURP", status);

        // should only get one notification even if register multiple times
        logln("... trying multiple registration");
        ls.addListener(&one, status);
        ls.addListener(&one, status);
        ls.addListener(&one, status);
        ls.addListener(&two, status);
        ls.registerInstance(new UnicodeString("Foo"), "en_FOO", status);
        logln("... registered foo");
    }
#if 0
    // same thread, so we can't callback within notification, unlike Java
    ServiceListener l3 = new ServiceListener() {
private int n;
public void serviceChanged(ICUService s) {
    logln("listener 3 report " + n++ + " service changed...");
    if (s.get("en_BOINK") == null) { // don't recurse on ourselves!!!
        logln("registering boink...");
        s.registerInstance("boink", "en_BOINK");
    }
}
    };
    ls.addListener(l3);
    logln("registering boo...");
    ls.registerInstance("Boo", "en_BOO");
#endif

    logln("...done");
}

class TestStringLocaleService : public ICULocaleService {
    public:
    virtual UObject* cloneInstance(UObject* instance) const {
        return instance ? new UnicodeString(*(UnicodeString*)instance) : NULL;
    }
};

void ICUServiceTest::testLocale() {
    UErrorCode status = U_ZERO_ERROR;
    TestStringLocaleService service;

    UnicodeString* root = new UnicodeString("root");
    UnicodeString* german = new UnicodeString("german");
    UnicodeString* germany = new UnicodeString("german_Germany");
    UnicodeString* japanese = new UnicodeString("japanese");
    UnicodeString* japan = new UnicodeString("japanese_Japan");

    service.registerInstance(root, "", status);
    service.registerInstance(german, "de", status);
    service.registerInstance(germany, Locale::getGermany(), status);
    service.registerInstance(japanese, (UnicodeString)"ja", TRUE, status);
    service.registerInstance(japan, Locale::getJapan(), status);

    {
        UErrorCode status = U_ZERO_ERROR;
        UnicodeString* target = (UnicodeString*)service.get("de_US", status);
        confirmEqual("test de_US", german, target);
        delete target;
    }

    {
        UErrorCode status = U_ZERO_ERROR;
        UnicodeString* target = (UnicodeString*)service.get("de_US", LocaleKey::KIND_ANY, status);
        confirmEqual("test de_US 2", german, target);
        delete target;
    }

    {
        UErrorCode status = U_ZERO_ERROR;
        UnicodeString* target = (UnicodeString*)service.get("de_US", 1234, status);
        confirmEqual("test de_US 3", german, target);
        delete target;
    }

    {
        UErrorCode status = U_ZERO_ERROR;
        Locale actualReturn;
        UnicodeString* target = (UnicodeString*)service.get("de_US", &actualReturn, status);
        confirmEqual("test de_US 5", german, target);
        confirmEqual("test de_US 6", &actualReturn, &Locale::getGerman());
        delete target;
    }

    {
        UErrorCode status = U_ZERO_ERROR;
        Locale actualReturn;
        UnicodeString* target = (UnicodeString*)service.get("de_US", LocaleKey::KIND_ANY, &actualReturn, status);
        confirmEqual("test de_US 7", &actualReturn, &Locale::getGerman());
        delete target;
    }

    {
        UErrorCode status = U_ZERO_ERROR;
        Locale actualReturn;
        UnicodeString* target = (UnicodeString*)service.get("de_US", 1234, &actualReturn, status);
        confirmEqual("test de_US 8", german, target);
        confirmEqual("test de_US 9", &actualReturn, &Locale::getGerman());
        delete target;
    }

    UnicodeString* one = new UnicodeString("one/de_US");
    UnicodeString* two = new UnicodeString("two/de_US");

    service.registerInstance(one, Locale("de_US"), 1, status);
    service.registerInstance(two, Locale("de_US"), 2, status);

    {
        UErrorCode status = U_ZERO_ERROR;
        UnicodeString* target = (UnicodeString*)service.get("de_US", 1, status);
        confirmEqual("test de_US kind 1", one, target);
        delete target;
    }
        
    {
        UErrorCode status = U_ZERO_ERROR;
        UnicodeString* target = (UnicodeString*)service.get("de_US", 2, status);
        confirmEqual("test de_US kind 2", two, target);
        delete target;
    }

    {
        UErrorCode status = U_ZERO_ERROR;
        UnicodeString* target = (UnicodeString*)service.get("de_US", status);
        confirmEqual("test de_US kind 3", german, target);
        delete target;
    }

    {
        UErrorCode status = U_ZERO_ERROR;
        UnicodeString english = "en";
        Locale localeResult;
        UnicodeString result;
        LocaleKey* lkey = LocaleKey::createWithCanonicalFallback(&english, NULL, 1234, status);
        logln("lkey prefix: " + lkey->prefix(result));
        result.remove();
        logln("lkey descriptor: " + lkey->currentDescriptor(result));
        result.remove();
        logln(UnicodeString("lkey current locale: ") + lkey->currentLocale(localeResult).getName());
        result.remove();

        lkey->fallback();
        logln("lkey descriptor 2: " + lkey->currentDescriptor(result));
        result.remove();

        lkey->fallback();
        logln("lkey descriptor 3: " + lkey->currentDescriptor(result));
        result.remove();
        delete lkey; // tentatively weiv
    }

    {
        UErrorCode status = U_ZERO_ERROR;
        UnicodeString* target = (UnicodeString*)service.get("za_PPP", status);
        confirmEqual("test zappp", root, target);
        delete target;
    }

    Locale loc = Locale::getDefault();
    Locale::setDefault(Locale::getJapanese(), status);
    {
        UErrorCode status = U_ZERO_ERROR;
        UnicodeString* target = (UnicodeString*)service.get("za_PPP", status);
        confirmEqual("test with ja locale", japanese, target);
        delete target;
    }

    {
        UErrorCode status = U_ZERO_ERROR;
        UVector ids(uhash_deleteUnicodeString, uhash_compareUnicodeString, 0, status);
        service.getVisibleIDs(ids, status);
        logln("all visible ids:");
        for (int i = 0; i < ids.size(); ++i) {
            const UnicodeString* id = (const UnicodeString*)ids[i];
            logln(*id);
        }
    }

    Locale::setDefault(loc, status);
    {
        UErrorCode status = U_ZERO_ERROR;
        UVector ids(uhash_deleteUnicodeString, uhash_compareUnicodeString, 0, status);
        service.getVisibleIDs(ids, status);
        logln("all visible ids:");
        for (int i = 0; i < ids.size(); ++i) {
            const UnicodeString* id = (const UnicodeString*)ids[i];
            logln(*id);
        }
    }

    {
        UErrorCode status = U_ZERO_ERROR;
        UnicodeString* target = (UnicodeString*)service.get("za_PPP", status);
        confirmEqual("test with en locale", root, target);
        delete target;
    }

    {
        UErrorCode status = U_ZERO_ERROR;
        StringEnumeration* locales = service.getAvailableLocales();
        if (locales) {
            confirmIdentical("test available locales", locales->count(status), 6);
            logln("locales: ");
            {
                const char* p;
                while ((p = locales->next(NULL, status))) {
                    logln(p);
                }
            }
            logln(" ");
            delete locales;
        } else {
            errln("could not create available locales");
        }
    }
}

class WrapFactory : public ICUServiceFactory {
    public:
    static const UnicodeString& getGreetingID() {
      if (greetingID == NULL) {
    greetingID = new UnicodeString("greeting");
      }
      return *greetingID;
    }

  static void cleanup() {
    delete greetingID;
    greetingID = NULL;
  }

    UObject* create(const ICUServiceKey& key, const ICUService* service, UErrorCode& status) const {
        if (U_SUCCESS(status)) {
            UnicodeString temp;
            if (key.currentID(temp).compare(getGreetingID()) == 0) {
                UnicodeString* previous = (UnicodeString*)service->getKey((ICUServiceKey&)key, NULL, this, status);
                if (previous) {
                    previous->insert(0, "A different greeting: \"");
                    previous->append("\"");
                    return previous;
                }
            }
        }
        return NULL;
    }

    void updateVisibleIDs(Hashtable& result, UErrorCode& status) const {
        if (U_SUCCESS(status)) {
            result.put("greeting", (void*)this, status);
        }
    }

    UnicodeString& getDisplayName(const UnicodeString& id, const Locale& /* locale */, UnicodeString& result) const {
        result.append("wrap '");
        result.append(id);
        result.append("'");
        return result;
    }

    /**
     * UObject boilerplate.
     */
    static UClassID getStaticClassID() { 
        return (UClassID)&fgClassID;
    }

    virtual UClassID getDynamicClassID() const {
        return getStaticClassID();
    }

    private:
    static const char fgClassID;
    static UnicodeString* greetingID;
};

UnicodeString* WrapFactory::greetingID = NULL;
const char WrapFactory::fgClassID = '\0';

void 
ICUServiceTest::testWrapFactory() 
{
    UnicodeString* greeting = new UnicodeString("Hello There");
    UnicodeString greetingID = "greeting";
    UErrorCode status = U_ZERO_ERROR;
    TestStringService service;
    service.registerInstance(greeting, greetingID, status);

    {
        UErrorCode status = U_ZERO_ERROR;
        UnicodeString* result = (UnicodeString*)service.get(greetingID, status);
        if (result) {
            logln("test one: " + *result);
            delete result;
        }
    }

    service.registerFactory(new WrapFactory(), status);
    {
        UErrorCode status = U_ZERO_ERROR;
        UnicodeString* result = (UnicodeString*)service.get(greetingID, status);
        UnicodeString target = "A different greeting: \"Hello There\"";
        confirmEqual("wrap test: ", result, &target);
        delete result;
    }

    WrapFactory::cleanup();
}

  // misc coverage tests
void ICUServiceTest::testCoverage() 
{
  // ICUServiceKey
  {
    UnicodeString temp;
    ICUServiceKey key("foobar");
    logln("ID: " + key.getID());
    logln("canonicalID: " + key.canonicalID(temp));
    logln("currentID: " + key.currentID(temp.remove()));
    logln("has fallback: " + UnicodeString(key.fallback() ? "true" : "false"));

    if (key.getDynamicClassID() != ICUServiceKey::getStaticClassID()) {
      errln("service key rtt failed.");
    }
  }

  // SimpleFactory
  {
    UErrorCode status = U_ZERO_ERROR;

    UnicodeString* obj = new UnicodeString("An Object");
    SimpleFactory* sf = new SimpleFactory(obj, "object");

    UnicodeString temp;
    logln(sf->getDisplayName("object", Locale::getDefault(), temp));

    if (sf->getDynamicClassID() != SimpleFactory::getStaticClassID()) {
      errln("simple factory rtti failed.");
    }

    // ICUService
        {
                TestStringService service;
                service.registerFactory(sf,     status);

                {
                        UnicodeString* result   = (UnicodeString*)service.get("object", status);
                        if (result) {
                                logln("object is: "     + *result);
                                delete result;
                        }       else {
                                errln("could not get object");
                        }
                }
        }
  }
  
  // ICUServiceKey
  {
      UErrorCode status = U_ZERO_ERROR;
          UnicodeString* howdy = new UnicodeString("Howdy");

          TestStringSimpleKeyService service;
          service.registerInstance(howdy, "Greetings", status);
          {
                  UnicodeString* result = (UnicodeString*)service.get("Greetings",      status);
                  if (result) {
                          logln("object is: "   + *result);
                          delete result;
                  }     else {
                          errln("could not get object");
                  }
          }

      UVector ids(uhash_deleteUnicodeString, uhash_compareUnicodeString, status);
          // yuck, this is awkward to use.  All because we pass null in an overload.
          // TODO: change this.
          UnicodeString str("Greet");
      service.getVisibleIDs(ids, &str, status);
      confirmIdentical("no fallback of greet", ids.size(), 0);
  }

  // ICULocaleService

  // LocaleKey
  {
    UnicodeString primary("en_US");
    UnicodeString fallback("ja_JP");
    UErrorCode status = U_ZERO_ERROR;
    LocaleKey* key = LocaleKey::createWithCanonicalFallback(&primary, &fallback, status);

    if (key->getDynamicClassID() != LocaleKey::getStaticClassID()) {
      errln("localekey rtti error");
    }

    if (!key->isFallbackOf("en_US_FOOBAR")) {
      errln("localekey should be fallback for en_US_FOOBAR");
    }
    if (!key->isFallbackOf("en_US")) {
      errln("localekey should be fallback for en_US");
    }
    if (key->isFallbackOf("en")) {
      errln("localekey should not be fallback for en");
    }

    do {
      Locale loc;
      logln(UnicodeString("current locale: ") + key->currentLocale(loc).getName());
      logln(UnicodeString("canonical locale: ") + key->canonicalLocale(loc).getName());
      logln(UnicodeString("is fallback of en: ") + (key->isFallbackOf("en") ? "true" : " false"));
    } while (key->fallback());
    delete key;

    // LocaleKeyFactory 
    key = LocaleKey::createWithCanonicalFallback(&primary, &fallback, status);

    UnicodeString result;
    LKFSubclass lkf(TRUE); // empty
    Hashtable table;

    UObject *obj = lkf.create(*key, NULL, status);
    logln("obj: " + UnicodeString(obj ? "obj" : "null"));
    logln(lkf.getDisplayName("en_US", Locale::getDefault(), result));
    lkf.updateVisibleIDs(table, status);
    delete obj;
    if (table.count() != 1) {
      errln("visible IDs does not contain en_US");
    }

    LKFSubclass invisibleLKF(FALSE);
    obj = lkf.create(*key, NULL, status);
    logln("obj: " + UnicodeString(obj ? "obj" : "null"));
    logln(invisibleLKF.getDisplayName("en_US", Locale::getDefault(), result.remove()));
    invisibleLKF.updateVisibleIDs(table, status);
    if (table.count() != 0) {
      errln("visible IDs contains en_US");
    }
    delete obj;
    delete key;

        key = LocaleKey::createWithCanonicalFallback(&primary, &fallback, 123, status);
        if (U_SUCCESS(status)) {
                UnicodeString str;
                key->currentDescriptor(str);
                key->parsePrefix(str);
                if (str != "123") {
                        errln("did not get expected prefix");
                }
                delete key;
        }

        // coverage, getSupportedIDs is either overridden or the calling method is
        LKFSubclass0 lkFactory;
        Hashtable table0;
        lkFactory.updateVisibleIDs(table0, status);
        if (table0.count() != 0) {
                errln("LKF returned non-empty hashtable");
        }


        // ResourceBundleFactory
    key = LocaleKey::createWithCanonicalFallback(&primary, &fallback, status);
        ICUResourceBundleFactory rbf;
        UObject* icurb = rbf.create(*key, NULL, status);
        if (icurb != NULL) {
                logln("got resource bundle for key");
                delete icurb;
        }
        delete key;
  }

 #if 0
 // ICUNotifier
  ICUNotifier nf = new ICUNSubclass();
  try {
    nf.addListener(null);
    errln("added null listener");
  }
  catch (NullPointerException e) {
    logln(e.getMessage());
  }
  catch (Exception e) {
    errln("got wrong exception");
  }

  try {
    nf.addListener(new WrongListener());
    errln("added wrong listener");
  }
  catch (InternalError e) {
    logln(e.getMessage());
  }
  catch (Exception e) {
    errln("got wrong exception");
  }

  try {
    nf.removeListener(null);
    errln("removed null listener");
  }
  catch (NullPointerException e) {
    logln(e.getMessage());
  }
  catch (Exception e) {
    errln("got wrong exception");
  }

  nf.removeListener(new MyListener());
  nf.notifyChanged();
  nf.addListener(new MyListener());
  nf.removeListener(new MyListener());
#endif
}


/* !UCONFIG_NO_SERVICE */
#endif


