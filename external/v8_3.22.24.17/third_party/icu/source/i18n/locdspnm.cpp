/*
*******************************************************************************
* Copyright (C) 2010, International Business Machines Corporation and         *
* others. All Rights Reserved.                                                *
*******************************************************************************
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/locdspnm.h"

#include "unicode/msgfmt.h"

#include "cmemory.h"
#include "cstring.h"
#include "ulocimp.h"
#include "ureslocs.h"

#include <stdarg.h>

/**
 * Concatenate a number of null-terminated strings to buffer, leaving a
 * null-terminated string.  The last argument should be the null pointer.
 * Return the length of the string in the buffer, not counting the trailing
 * null.  Return -1 if there is an error (buffer is null, or buflen < 1).
 */
static int32_t ncat(char *buffer, uint32_t buflen, ...) {
  va_list args;
  char *str;
  char *p = buffer;
  const char* e = buffer + buflen - 1;

  if (buffer == NULL || buflen < 1) {
    return -1;
  }

  va_start(args, buflen);
  while ((str = va_arg(args, char *))) {
    char c;
    while (p != e && (c = *str++)) {
      *p++ = c;
    }
  }
  *p = 0;
  va_end(args);

  return p - buffer;
}

U_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////

// Access resource data for locale components.
// Wrap code in uloc.c for now.
class ICUDataTable {
  const char* path;
  Locale locale;

public:
  ICUDataTable(const char* path, const Locale& locale);
  ~ICUDataTable();

  const Locale& getLocale();

  UnicodeString& get(const char* tableKey, const char* itemKey,
                     UnicodeString& result) const;
  UnicodeString& get(const char* tableKey, const char* subTableKey, const char* itemKey,
                     UnicodeString& result) const;

  UnicodeString& getNoFallback(const char* tableKey, const char* itemKey,
                               UnicodeString &result) const;
  UnicodeString& getNoFallback(const char* tableKey, const char* subTableKey, const char* itemKey,
                               UnicodeString &result) const;
};

inline UnicodeString &
ICUDataTable::get(const char* tableKey, const char* itemKey, UnicodeString& result) const {
  return get(tableKey, NULL, itemKey, result);
}

inline UnicodeString &
ICUDataTable::getNoFallback(const char* tableKey, const char* itemKey, UnicodeString& result) const {
  return getNoFallback(tableKey, NULL, itemKey, result);
}

ICUDataTable::ICUDataTable(const char* path, const Locale& locale)
  : path(NULL), locale(Locale::getRoot())
{
  if (path) {
    int32_t len = uprv_strlen(path);
    this->path = (const char*) uprv_malloc(len + 1);
    if (this->path) {
      uprv_strcpy((char *)this->path, path);
      this->locale = locale;
    }
  }
}

ICUDataTable::~ICUDataTable() {
  if (path) {
    uprv_free((void*) path);
    path = NULL;
  }
}

const Locale&
ICUDataTable::getLocale() {
  return locale;
}

UnicodeString &
ICUDataTable::get(const char* tableKey, const char* subTableKey, const char* itemKey,
                  UnicodeString &result) const {
  UErrorCode status = U_ZERO_ERROR;
  int32_t len = 0;

  const UChar *s = uloc_getTableStringWithFallback(path, locale.getName(),
                                                   tableKey, subTableKey, itemKey,
                                                   &len, &status);
  if (U_SUCCESS(status)) {
    return result.setTo(s, len);
  }
  return result.setTo(UnicodeString(itemKey, -1, US_INV));
}

UnicodeString &
ICUDataTable::getNoFallback(const char* tableKey, const char* subTableKey, const char* itemKey,
                            UnicodeString& result) const {
  UErrorCode status = U_ZERO_ERROR;
  int32_t len = 0;

  const UChar *s = uloc_getTableStringWithFallback(path, locale.getName(),
                                                   tableKey, subTableKey, itemKey,
                                                   &len, &status);
  if (U_SUCCESS(status)) {
    return result.setTo(s, len);
  }

  result.setToBogus();
  return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

UOBJECT_DEFINE_NO_RTTI_IMPLEMENTATION(LocaleDisplayNames)

////////////////////////////////////////////////////////////////////////////////////////////////////

#if 0  // currently unused

class DefaultLocaleDisplayNames : public LocaleDisplayNames {
  UDialectHandling dialectHandling;

public:
  // constructor
  DefaultLocaleDisplayNames(UDialectHandling dialectHandling);

  virtual ~DefaultLocaleDisplayNames();

  virtual const Locale& getLocale() const;
  virtual UDialectHandling getDialectHandling() const;
  virtual UnicodeString& localeDisplayName(const Locale& locale,
                                           UnicodeString& result) const;
  virtual UnicodeString& localeDisplayName(const char* localeId,
                                           UnicodeString& result) const;
  virtual UnicodeString& languageDisplayName(const char* lang,
                                             UnicodeString& result) const;
  virtual UnicodeString& scriptDisplayName(const char* script,
                                           UnicodeString& result) const;
  virtual UnicodeString& scriptDisplayName(UScriptCode scriptCode,
                                           UnicodeString& result) const;
  virtual UnicodeString& regionDisplayName(const char* region,
                                           UnicodeString& result) const;
  virtual UnicodeString& variantDisplayName(const char* variant,
                                            UnicodeString& result) const;
  virtual UnicodeString& keyDisplayName(const char* key,
                                        UnicodeString& result) const;
  virtual UnicodeString& keyValueDisplayName(const char* key,
                                             const char* value,
                                             UnicodeString& result) const;
};

DefaultLocaleDisplayNames::DefaultLocaleDisplayNames(UDialectHandling dialectHandling)
    : dialectHandling(dialectHandling) {
}

DefaultLocaleDisplayNames::~DefaultLocaleDisplayNames() {
}

const Locale&
DefaultLocaleDisplayNames::getLocale() const {
  return Locale::getRoot();
}

UDialectHandling
DefaultLocaleDisplayNames::getDialectHandling() const {
  return dialectHandling;
}

UnicodeString&
DefaultLocaleDisplayNames::localeDisplayName(const Locale& locale,
                                             UnicodeString& result) const {
  return result = UnicodeString(locale.getName(), -1, US_INV);
}

UnicodeString&
DefaultLocaleDisplayNames::localeDisplayName(const char* localeId,
                                             UnicodeString& result) const {
  return result = UnicodeString(localeId, -1, US_INV);
}

UnicodeString&
DefaultLocaleDisplayNames::languageDisplayName(const char* lang,
                                               UnicodeString& result) const {
  return result = UnicodeString(lang, -1, US_INV);
}

UnicodeString&
DefaultLocaleDisplayNames::scriptDisplayName(const char* script,
                                             UnicodeString& result) const {
  return result = UnicodeString(script, -1, US_INV);
}

UnicodeString&
DefaultLocaleDisplayNames::scriptDisplayName(UScriptCode scriptCode,
                                             UnicodeString& result) const {
  const char* name = uscript_getName(scriptCode);
  if (name) {
    return result = UnicodeString(name, -1, US_INV);
  }
  return result.remove();
}

UnicodeString&
DefaultLocaleDisplayNames::regionDisplayName(const char* region,
                                             UnicodeString& result) const {
  return result = UnicodeString(region, -1, US_INV);
}

UnicodeString&
DefaultLocaleDisplayNames::variantDisplayName(const char* variant,
                                              UnicodeString& result) const {
  return result = UnicodeString(variant, -1, US_INV);
}

UnicodeString&
DefaultLocaleDisplayNames::keyDisplayName(const char* key,
                                          UnicodeString& result) const {
  return result = UnicodeString(key, -1, US_INV);
}

UnicodeString&
DefaultLocaleDisplayNames::keyValueDisplayName(const char* /* key */,
                                               const char* value,
                                               UnicodeString& result) const {
  return result = UnicodeString(value, -1, US_INV);
}

#endif  // currently unused class DefaultLocaleDisplayNames

////////////////////////////////////////////////////////////////////////////////////////////////////

class LocaleDisplayNamesImpl : public LocaleDisplayNames {
  Locale locale;
  UDialectHandling dialectHandling;
  ICUDataTable langData;
  ICUDataTable regionData;
  UnicodeString sep;
  MessageFormat *format;

public:
  // constructor
  LocaleDisplayNamesImpl(const Locale& locale, UDialectHandling dialectHandling);
  virtual ~LocaleDisplayNamesImpl();

  virtual const Locale& getLocale() const;
  virtual UDialectHandling getDialectHandling() const;

  virtual UnicodeString& localeDisplayName(const Locale& locale,
                                           UnicodeString& result) const;
  virtual UnicodeString& localeDisplayName(const char* localeId,
                                           UnicodeString& result) const;
  virtual UnicodeString& languageDisplayName(const char* lang,
                                             UnicodeString& result) const;
  virtual UnicodeString& scriptDisplayName(const char* script,
                                           UnicodeString& result) const;
  virtual UnicodeString& scriptDisplayName(UScriptCode scriptCode,
                                           UnicodeString& result) const;
  virtual UnicodeString& regionDisplayName(const char* region,
                                           UnicodeString& result) const;
  virtual UnicodeString& variantDisplayName(const char* variant,
                                            UnicodeString& result) const;
  virtual UnicodeString& keyDisplayName(const char* key,
                                        UnicodeString& result) const;
  virtual UnicodeString& keyValueDisplayName(const char* key,
                                             const char* value,
                                             UnicodeString& result) const;
private:
  UnicodeString& localeIdName(const char* localeId,
                              UnicodeString& result) const;
  UnicodeString& appendWithSep(UnicodeString& buffer, const UnicodeString& src) const;
};

LocaleDisplayNamesImpl::LocaleDisplayNamesImpl(const Locale& locale,
                                               UDialectHandling dialectHandling)
  : dialectHandling(dialectHandling)
  , langData(U_ICUDATA_LANG, locale)
  , regionData(U_ICUDATA_REGION, locale)
  , format(NULL)
{
  LocaleDisplayNamesImpl *nonConstThis = (LocaleDisplayNamesImpl *)this;
  nonConstThis->locale = langData.getLocale() == Locale::getRoot()
    ? regionData.getLocale()
    : langData.getLocale();

  langData.getNoFallback("localeDisplayPattern", "separator", sep);
  if (sep.isBogus()) {
    sep = UnicodeString(", ", -1, US_INV);
  }

  UnicodeString pattern;
  langData.getNoFallback("localeDisplayPattern", "pattern", pattern);
  if (pattern.isBogus()) {
    pattern = UnicodeString("{0} ({1})", -1, US_INV);
  }
  UErrorCode status = U_ZERO_ERROR;
  format = new MessageFormat(pattern, status);
}

LocaleDisplayNamesImpl::~LocaleDisplayNamesImpl() {
  delete format;
}

const Locale&
LocaleDisplayNamesImpl::getLocale() const {
  return locale;
}

UDialectHandling
LocaleDisplayNamesImpl::getDialectHandling() const {
  return dialectHandling;
}

UnicodeString&
LocaleDisplayNamesImpl::localeDisplayName(const Locale& locale,
                                          UnicodeString& result) const {
  UnicodeString resultName;

  const char* lang = locale.getLanguage();
  if (uprv_strlen(lang) == 0) {
    lang = "root";
  }
  const char* script = locale.getScript();
  const char* country = locale.getCountry();
  const char* variant = locale.getVariant();

  UBool hasScript = uprv_strlen(script) > 0;
  UBool hasCountry = uprv_strlen(country) > 0;
  UBool hasVariant = uprv_strlen(variant) > 0;

  if (dialectHandling == ULDN_DIALECT_NAMES) {
    char buffer[ULOC_FULLNAME_CAPACITY];
    do { // loop construct is so we can break early out of search
      if (hasScript && hasCountry) {
        ncat(buffer, ULOC_FULLNAME_CAPACITY, lang, "_", script, "_", country, (char *)0);
        localeIdName(buffer, resultName);
        if (!resultName.isBogus()) {
          hasScript = FALSE;
          hasCountry = FALSE;
          break;
        }
      }
      if (hasScript) {
        ncat(buffer, ULOC_FULLNAME_CAPACITY, lang, "_", script, (char *)0);
        localeIdName(buffer, resultName);
        if (!resultName.isBogus()) {
          hasScript = FALSE;
          break;
        }
      }
      if (hasCountry) {
        ncat(buffer, ULOC_FULLNAME_CAPACITY, lang, "_", country, (char*)0);
        localeIdName(buffer, resultName);
        if (!resultName.isBogus()) {
          hasCountry = FALSE;
          break;
        }
      }
    } while (FALSE);
  }
  if (resultName.isBogus() || resultName.isEmpty()) {
    localeIdName(lang, resultName);
  }

  UnicodeString resultRemainder;
  UnicodeString temp;
  StringEnumeration *e = NULL;
  UErrorCode status = U_ZERO_ERROR;

  if (hasScript) {
    resultRemainder.append(scriptDisplayName(script, temp));
  }
  if (hasCountry) {
    appendWithSep(resultRemainder, regionDisplayName(country, temp));
  }
  if (hasVariant) {
    appendWithSep(resultRemainder, variantDisplayName(variant, temp));
  }

  e = locale.createKeywords(status);
  if (e && U_SUCCESS(status)) {
    UnicodeString temp2;
    char value[ULOC_KEYWORD_AND_VALUES_CAPACITY]; // sigh, no ULOC_VALUE_CAPACITY
    const char* key;
    while ((key = e->next((int32_t *)0, status)) != NULL) {
      locale.getKeywordValue(key, value, ULOC_KEYWORD_AND_VALUES_CAPACITY, status);
      appendWithSep(resultRemainder, keyDisplayName(key, temp))
          .append("=")
          .append(keyValueDisplayName(key, value, temp2));
    }
    delete e;
  }

  if (!resultRemainder.isEmpty()) {
    Formattable data[] = {
      resultName,
      resultRemainder
    };
    FieldPosition fpos;
    status = U_ZERO_ERROR;
    format->format(data, 2, result, fpos, status);
    return result;
  }

  return result = resultName;
}

UnicodeString&
LocaleDisplayNamesImpl::appendWithSep(UnicodeString& buffer, const UnicodeString& src) const {
  if (!buffer.isEmpty()) {
    buffer.append(sep);
  }
  buffer.append(src);
  return buffer;
}

UnicodeString&
LocaleDisplayNamesImpl::localeDisplayName(const char* localeId,
                                          UnicodeString& result) const {
  return localeDisplayName(Locale(localeId), result);
}

UnicodeString&
LocaleDisplayNamesImpl::localeIdName(const char* localeId,
                                     UnicodeString& result) const {
  return langData.getNoFallback("Languages", localeId, result);
}

UnicodeString&
LocaleDisplayNamesImpl::languageDisplayName(const char* lang,
                                            UnicodeString& result) const {
  if (uprv_strcmp("root", lang) == 0 || uprv_strchr(lang, '_') != NULL) {
    return result = UnicodeString(lang, -1, US_INV);
  }
  return langData.get("Languages", lang, result);
}

UnicodeString&
LocaleDisplayNamesImpl::scriptDisplayName(const char* script,
                                          UnicodeString& result) const {
  return langData.get("Scripts", script, result);
}

UnicodeString&
LocaleDisplayNamesImpl::scriptDisplayName(UScriptCode scriptCode,
                                          UnicodeString& result) const {
  const char* name = uscript_getName(scriptCode);
  return langData.get("Scripts", name, result);
}

UnicodeString&
LocaleDisplayNamesImpl::regionDisplayName(const char* region,
                                          UnicodeString& result) const {
  return regionData.get("Countries", region, result);
}

UnicodeString&
LocaleDisplayNamesImpl::variantDisplayName(const char* variant,
                                           UnicodeString& result) const {
  return langData.get("Variants", variant, result);
}

UnicodeString&
LocaleDisplayNamesImpl::keyDisplayName(const char* key,
                                       UnicodeString& result) const {
  return langData.get("Keys", key, result);
}

UnicodeString&
LocaleDisplayNamesImpl::keyValueDisplayName(const char* key,
                                            const char* value,
                                            UnicodeString& result) const {
  return langData.get("Types", key, value, result);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

LocaleDisplayNames*
LocaleDisplayNames::createInstance(const Locale& locale,
                                   UDialectHandling dialectHandling) {
  return new LocaleDisplayNamesImpl(locale, dialectHandling);
}

U_NAMESPACE_END

////////////////////////////////////////////////////////////////////////////////////////////////////

U_NAMESPACE_USE

U_DRAFT ULocaleDisplayNames * U_EXPORT2
uldn_open(const char * locale,
          UDialectHandling dialectHandling,
          UErrorCode *pErrorCode) {
  if (U_FAILURE(*pErrorCode)) {
    return 0;
  }
  if (locale == NULL) {
    locale = uloc_getDefault();
  }
  return (ULocaleDisplayNames *)LocaleDisplayNames::createInstance(Locale(locale), dialectHandling);
}

U_DRAFT void U_EXPORT2
uldn_close(ULocaleDisplayNames *ldn) {
  delete (LocaleDisplayNames *)ldn;
}

U_DRAFT const char * U_EXPORT2
uldn_getLocale(const ULocaleDisplayNames *ldn) {
  if (ldn) {
    return ((const LocaleDisplayNames *)ldn)->getLocale().getName();
  }
  return NULL;
}

U_DRAFT UDialectHandling U_EXPORT2
uldn_getDialectHandling(const ULocaleDisplayNames *ldn) {
  if (ldn) {
    return ((const LocaleDisplayNames *)ldn)->getDialectHandling();
  }
  return ULDN_STANDARD_NAMES;
}

U_DRAFT int32_t U_EXPORT2
uldn_localeDisplayName(const ULocaleDisplayNames *ldn,
                       const char *locale,
                       UChar *result,
                       int32_t maxResultSize,
                       UErrorCode *pErrorCode) {
  if (U_FAILURE(*pErrorCode)) {
    return 0;
  }
  if (ldn == NULL || locale == NULL || (result == NULL && maxResultSize > 0) || maxResultSize < 0) {
    *pErrorCode = U_ILLEGAL_ARGUMENT_ERROR;
    return 0;
  }
  UnicodeString temp(result, 0, maxResultSize);
  ((const LocaleDisplayNames *)ldn)->localeDisplayName(locale, temp);
  return temp.extract(result, maxResultSize, *pErrorCode);
}

U_DRAFT int32_t U_EXPORT2
uldn_languageDisplayName(const ULocaleDisplayNames *ldn,
                         const char *lang,
                         UChar *result,
                         int32_t maxResultSize,
                         UErrorCode *pErrorCode) {
  if (U_FAILURE(*pErrorCode)) {
    return 0;
  }
  if (ldn == NULL || lang == NULL || (result == NULL && maxResultSize > 0) || maxResultSize < 0) {
    *pErrorCode = U_ILLEGAL_ARGUMENT_ERROR;
    return 0;
  }
  UnicodeString temp(result, 0, maxResultSize);
  ((const LocaleDisplayNames *)ldn)->languageDisplayName(lang, temp);
  return temp.extract(result, maxResultSize, *pErrorCode);
}

U_DRAFT int32_t U_EXPORT2
uldn_scriptDisplayName(const ULocaleDisplayNames *ldn,
                       const char *script,
                       UChar *result,
                       int32_t maxResultSize,
                       UErrorCode *pErrorCode) {
  if (U_FAILURE(*pErrorCode)) {
    return 0;
  }
  if (ldn == NULL || script == NULL || (result == NULL && maxResultSize > 0) || maxResultSize < 0) {
    *pErrorCode = U_ILLEGAL_ARGUMENT_ERROR;
    return 0;
  }
  UnicodeString temp(result, 0, maxResultSize);
  ((const LocaleDisplayNames *)ldn)->scriptDisplayName(script, temp);
  return temp.extract(result, maxResultSize, *pErrorCode);
}

U_DRAFT int32_t U_EXPORT2
uldn_scriptCodeDisplayName(const ULocaleDisplayNames *ldn,
                           UScriptCode scriptCode,
                           UChar *result,
                           int32_t maxResultSize,
                           UErrorCode *pErrorCode) {
  return uldn_scriptDisplayName(ldn, uscript_getName(scriptCode), result, maxResultSize, pErrorCode);
}

U_DRAFT int32_t U_EXPORT2
uldn_regionDisplayName(const ULocaleDisplayNames *ldn,
                       const char *region,
                       UChar *result,
                       int32_t maxResultSize,
                       UErrorCode *pErrorCode) {
  if (U_FAILURE(*pErrorCode)) {
    return 0;
  }
  if (ldn == NULL || region == NULL || (result == NULL && maxResultSize > 0) || maxResultSize < 0) {
    *pErrorCode = U_ILLEGAL_ARGUMENT_ERROR;
    return 0;
  }
  UnicodeString temp(result, 0, maxResultSize);
  ((const LocaleDisplayNames *)ldn)->regionDisplayName(region, temp);
  return temp.extract(result, maxResultSize, *pErrorCode);
}

U_DRAFT int32_t U_EXPORT2
uldn_variantDisplayName(const ULocaleDisplayNames *ldn,
                        const char *variant,
                        UChar *result,
                        int32_t maxResultSize,
                        UErrorCode *pErrorCode) {
  if (U_FAILURE(*pErrorCode)) {
    return 0;
  }
  if (ldn == NULL || variant == NULL || (result == NULL && maxResultSize > 0) || maxResultSize < 0) {
    *pErrorCode = U_ILLEGAL_ARGUMENT_ERROR;
    return 0;
  }
  UnicodeString temp(result, 0, maxResultSize);
  ((const LocaleDisplayNames *)ldn)->variantDisplayName(variant, temp);
  return temp.extract(result, maxResultSize, *pErrorCode);
}

U_DRAFT int32_t U_EXPORT2
uldn_keyDisplayName(const ULocaleDisplayNames *ldn,
                    const char *key,
                    UChar *result,
                    int32_t maxResultSize,
                    UErrorCode *pErrorCode) {
  if (U_FAILURE(*pErrorCode)) {
    return 0;
  }
  if (ldn == NULL || key == NULL || (result == NULL && maxResultSize > 0) || maxResultSize < 0) {
    *pErrorCode = U_ILLEGAL_ARGUMENT_ERROR;
    return 0;
  }
  UnicodeString temp(result, 0, maxResultSize);
  ((const LocaleDisplayNames *)ldn)->keyDisplayName(key, temp);
  return temp.extract(result, maxResultSize, *pErrorCode);
}

U_DRAFT int32_t U_EXPORT2
uldn_keyValueDisplayName(const ULocaleDisplayNames *ldn,
                         const char *key,
                         const char *value,
                         UChar *result,
                         int32_t maxResultSize,
                         UErrorCode *pErrorCode) {
  if (U_FAILURE(*pErrorCode)) {
    return 0;
  }
  if (ldn == NULL || key == NULL || value == NULL || (result == NULL && maxResultSize > 0)
      || maxResultSize < 0) {
    *pErrorCode = U_ILLEGAL_ARGUMENT_ERROR;
    return 0;
  }
  UnicodeString temp(result, 0, maxResultSize);
  ((const LocaleDisplayNames *)ldn)->keyValueDisplayName(key, value, temp);
  return temp.extract(result, maxResultSize, *pErrorCode);
}

#endif
