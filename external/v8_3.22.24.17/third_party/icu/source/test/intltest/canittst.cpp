/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 2002-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************
 *
 * @author Mark E. Davis
 * @author Vladimir Weinstein
 */

#include "unicode/utypes.h"

#if !UCONFIG_NO_NORMALIZATION

#include "intltest.h"
#include "cstring.h"
#include "canittst.h"
#include "unicode/caniter.h"
#include "unicode/normlzr.h"
#include "unicode/uchar.h"
#include "hash.h"

#define ARRAY_LENGTH(array) ((int32_t)(sizeof (array) / sizeof (*array)))

#define CASE(id,test) case id:                          \
                          name = #test;                 \
                          if (exec) {                   \
                              logln(#test "---");       \
                              logln((UnicodeString)""); \
                              test();                   \
                          }                             \
                          break

void CanonicalIteratorTest::runIndexedTest(int32_t index, UBool exec,
                                         const char* &name, char* /*par*/) {
    switch (index) {
        CASE(0, TestBasic);
        CASE(1, TestExhaustive);
        CASE(2, TestAPI);
      default: name = ""; break;
    }
}

/**
 * Convert Java-style strings with \u Unicode escapes into UnicodeString objects
static UnicodeString str(const char *input)
{
    UnicodeString str(input, ""); // Invariant conversion
    return str.unescape();
}
 */


CanonicalIteratorTest::CanonicalIteratorTest() :
nameTrans(NULL), hexTrans(NULL)
{
}

CanonicalIteratorTest::~CanonicalIteratorTest()
{
#if !UCONFIG_NO_TRANSLITERATION
  if(nameTrans != NULL) {
    delete(nameTrans);
  }
  if(hexTrans != NULL) {
    delete(hexTrans);
  }
#endif
}

void CanonicalIteratorTest::TestExhaustive() {
    UErrorCode status = U_ZERO_ERROR;
    CanonicalIterator it("", status);
    if (U_FAILURE(status)) {
        dataerrln("Error creating CanonicalIterator: %s", u_errorName(status));
        return;
    }
    UChar32 i = 0;
    UnicodeString s;
    // Test static and dynamic class IDs
    if(it.getDynamicClassID() != CanonicalIterator::getStaticClassID()){
        errln("CanonicalIterator::getStaticClassId ! = CanonicalIterator.getDynamicClassID");
    }
    for (i = 0; i < 0x10FFFF; quick?i+=0x10:++i) {
        //for (i = 0xae00; i < 0xaf00; ++i) {
        
        if ((i % 0x100) == 0) {
            logln("Testing U+%06X", i);
        }
        
        // skip characters we know don't have decomps
        int8_t type = u_charType(i);
        if (type == U_UNASSIGNED || type == U_PRIVATE_USE_CHAR
            || type == U_SURROGATE) continue;
        
        s = i;
        characterTest(s, i, it);

        s += (UChar32)0x0345; //"\\u0345";
        characterTest(s, i, it);
    }
}

void CanonicalIteratorTest::TestBasic() {

    UErrorCode status = U_ZERO_ERROR;

    static const char * const testArray[][2] = {
        {"\\u00C5d\\u0307\\u0327", "A\\u030Ad\\u0307\\u0327, A\\u030Ad\\u0327\\u0307, A\\u030A\\u1E0B\\u0327, "
            "A\\u030A\\u1E11\\u0307, \\u00C5d\\u0307\\u0327, \\u00C5d\\u0327\\u0307, "
            "\\u00C5\\u1E0B\\u0327, \\u00C5\\u1E11\\u0307, \\u212Bd\\u0307\\u0327, "
            "\\u212Bd\\u0327\\u0307, \\u212B\\u1E0B\\u0327, \\u212B\\u1E11\\u0307"},
        {"\\u010d\\u017E", "c\\u030Cz\\u030C, c\\u030C\\u017E, \\u010Dz\\u030C, \\u010D\\u017E"},
        {"x\\u0307\\u0327", "x\\u0307\\u0327, x\\u0327\\u0307, \\u1E8B\\u0327"},
    };
    
#if 0
    // This is not interesting for C/C++ as the data is already built beforehand
    // check build
    UnicodeSet ss = CanonicalIterator.getSafeStart();
    logln("Safe Start: " + ss.toPattern(true));
    ss = CanonicalIterator.getStarts('a');
    expectEqual("Characters with 'a' at the start of their decomposition: ", "", CanonicalIterator.getStarts('a'),
        new UnicodeSet("[\u00E0-\u00E5\u0101\u0103\u0105\u01CE\u01DF\u01E1\u01FB"
        + "\u0201\u0203\u0227\u1E01\u1EA1\u1EA3\u1EA5\u1EA7\u1EA9\u1EAB\u1EAD\u1EAF\u1EB1\u1EB3\u1EB5\u1EB7]")
            );
#endif

    // check permute
    // NOTE: we use a TreeSet below to sort the output, which is not guaranteed to be sorted!

    Hashtable *permutations = new Hashtable(FALSE, status);
    permutations->setValueDeleter(uhash_deleteUnicodeString);
    UnicodeString toPermute("ABC");

    CanonicalIterator::permute(toPermute, FALSE, permutations, status);

    logln("testing permutation");
  
    expectEqual("Simple permutation ", "", collectionToString(permutations), "ABC, ACB, BAC, BCA, CAB, CBA");

    delete permutations;
    
    // try samples
    logln("testing samples");
    Hashtable *set = new Hashtable(FALSE, status);
    set->setValueDeleter(uhash_deleteUnicodeString);
    int32_t i = 0;
    CanonicalIterator it("", status);
    if(U_SUCCESS(status)) {
      for (i = 0; i < ARRAY_LENGTH(testArray); ++i) {
          //logln("Results for: " + name.transliterate(testArray[i]));
          UnicodeString testStr = CharsToUnicodeString(testArray[i][0]);
          it.setSource(testStr, status);
          set->removeAll();
          for (;;) {
              //UnicodeString *result = new UnicodeString(it.next());
              UnicodeString result(it.next());
              if (result.isBogus()) {
                  break;
              }
              set->put(result, new UnicodeString(result), status); // Add result to the table
              //logln(++counter + ": " + hex.transliterate(result));
              //logln(" = " + name.transliterate(result));
          }
          expectEqual(i + ": ", testStr, collectionToString(set), CharsToUnicodeString(testArray[i][1]));

      }
    } else {
      dataerrln("Couldn't instantiate canonical iterator. Error: %s", u_errorName(status));
    }
    delete set;
}

void CanonicalIteratorTest::characterTest(UnicodeString &s, UChar32 ch, CanonicalIterator &it)
{
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString decomp, comp;
    UBool gotDecomp = FALSE;
    UBool gotComp = FALSE;
    UBool gotSource = FALSE;

    Normalizer::decompose(s, FALSE, 0, decomp, status);
    Normalizer::compose(s, FALSE, 0, comp, status);
    
    // skip characters that don't have either decomp.
    // need quick test for this!
    if (s == decomp && s == comp) {
        return;
    }
    
    it.setSource(s, status);
    
    for (;;) {
        UnicodeString item = it.next();
        if (item.isBogus()) break;
        if (item == s) gotSource = TRUE;
        if (item == decomp) gotDecomp = TRUE;
        if (item == comp) gotComp = TRUE;
    }
    
    if (!gotSource || !gotDecomp || !gotComp) {
        errln("FAIL CanonicalIterator: " + s + (int)ch);
    }
}

void CanonicalIteratorTest::expectEqual(const UnicodeString &message, const UnicodeString &item, const UnicodeString &a, const UnicodeString &b) {
    if (!(a==b)) {
        errln("FAIL: " + message + getReadable(item));
        errln("\t" + getReadable(a));
        errln("\t" + getReadable(b));
    } else {
        logln("Checked: " + message + getReadable(item));
        logln("\t" + getReadable(a));
        logln("\t" + getReadable(b));
    }
}

UnicodeString CanonicalIteratorTest::getReadable(const UnicodeString &s) {
  UErrorCode status = U_ZERO_ERROR;
  UnicodeString result = "[";
    if (s.length() == 0) return "";
    // set up for readable display
#if !UCONFIG_NO_TRANSLITERATION
    if(verbose) {
      if (nameTrans == NULL)
          nameTrans = Transliterator::createInstance("[^\\ -\\u007F] name", UTRANS_FORWARD, status);
      UnicodeString sName = s;
      nameTrans->transliterate(sName);
      result += sName;
      result += ";";
    }
    if (hexTrans == NULL)
        hexTrans = Transliterator::createInstance("[^\\ -\\u007F] hex", UTRANS_FORWARD, status);
#endif
    UnicodeString sHex = s;
#if !UCONFIG_NO_TRANSLITERATION
    if(hexTrans) { // maybe there is no data and transliterator cannot be instantiated
      hexTrans->transliterate(sHex);
    }
#endif
    result += sHex;
    result += "]";
    return result;
    //return "[" + (verbose ? name->transliterate(s) + "; " : "") + hex->transliterate(s) + "]";
}

U_CFUNC int U_CALLCONV
compareUnicodeStrings(const void *s1, const void *s2) {
  UnicodeString **st1 = (UnicodeString **)s1;
  UnicodeString **st2 = (UnicodeString **)s2;

  return (*st1)->compare(**st2);
}


UnicodeString CanonicalIteratorTest::collectionToString(Hashtable *col) {
    UnicodeString result;

    // Iterate over the Hashtable, then qsort.

    UnicodeString **resArray = new UnicodeString*[col->count()];
    int32_t i = 0;

    const UHashElement *ne = NULL;
    int32_t el = -1;
    //Iterator it = basic.iterator();
    ne = col->nextElement(el);
    //while (it.hasNext()) 
    while (ne != NULL) {
      //String item = (String) it.next();
      UnicodeString *item = (UnicodeString *)(ne->value.pointer);
      resArray[i++] = item;
      ne = col->nextElement(el);
    }

    for(i = 0; i<col->count(); ++i) {
      logln(*resArray[i]);
    }

    qsort(resArray, col->count(), sizeof(UnicodeString *), compareUnicodeStrings);

    result = *resArray[0];

    for(i = 1; i<col->count(); ++i) {
      result += ", ";
      result += *resArray[i];
    }

/*
    Iterator it = col.iterator();
    while (it.hasNext()) {
        if (result.length() != 0) result.append(", ");
        result.append(it.next().toString());
    }
*/

    delete [] resArray;

    return result;
}

void CanonicalIteratorTest::TestAPI() {
  UErrorCode status = U_ZERO_ERROR;
  // Test reset and getSource
  UnicodeString start("ljubav");
  logln("Testing CanonicalIterator::getSource");
  logln("Instantiating canonical iterator with string "+start);
  CanonicalIterator can(start, status);
  if (U_FAILURE(status)) {
      dataerrln("Error creating CanonicalIterator: %s", u_errorName(status));
      return;
  }
  UnicodeString source = can.getSource();
  logln("CanonicalIterator::getSource returned "+source);
  if(start != source) {
    errln("CanonicalIterator.getSource() didn't return the starting string. Expected "+start+", got "+source);
  }
  logln("Testing CanonicalIterator::reset");
  UnicodeString next = can.next();
  logln("CanonicalIterator::next returned "+next);

  can.reset();

  UnicodeString afterReset = can.next();
  logln("After reset, CanonicalIterator::next returned "+afterReset);

  if(next != afterReset) {
    errln("Next after instantiation ("+next+") is different from next after reset ("+afterReset+").");
  }
  
  logln("Testing getStaticClassID and getDynamicClassID");
  if(can.getDynamicClassID() != CanonicalIterator::getStaticClassID()){
      errln("RTTI failed for CanonicalIterator getDynamicClassID != getStaticClassID");
  }
}

#endif /* #if !UCONFIG_NO_NORMALIZATION */
