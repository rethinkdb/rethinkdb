/*
******************************************************************************
*   Copyright (C) 1997-2008, International Business Machines
*   Corporation and others.  All Rights Reserved.
******************************************************************************
*   file name:  nfrule.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
* Modification history
* Date        Name      Comments
* 10/11/2001  Doug      Ported from ICU4J
*/

#include "nfrule.h"

#if U_HAVE_RBNF

#include "unicode/rbnf.h"
#include "unicode/tblcoll.h"
#include "unicode/coleitr.h"
#include "unicode/uchar.h"
#include "nfrs.h"
#include "nfrlist.h"
#include "nfsubs.h"

#include "util.h"

U_NAMESPACE_BEGIN

NFRule::NFRule(const RuleBasedNumberFormat* _rbnf)
  : baseValue((int32_t)0)
  , radix(0)
  , exponent(0)
  , ruleText()
  , sub1(NULL)
  , sub2(NULL)
  , formatter(_rbnf)
{
}

NFRule::~NFRule()
{
  delete sub1;
  delete sub2;
}

static const UChar gLeftBracket = 0x005b;
static const UChar gRightBracket = 0x005d;
static const UChar gColon = 0x003a;
static const UChar gZero = 0x0030;
static const UChar gNine = 0x0039;
static const UChar gSpace = 0x0020;
static const UChar gSlash = 0x002f;
static const UChar gGreaterThan = 0x003e;
static const UChar gLessThan = 0x003c;
static const UChar gComma = 0x002c;
static const UChar gDot = 0x002e;
static const UChar gTick = 0x0027;
//static const UChar gMinus = 0x002d;
static const UChar gSemicolon = 0x003b;

static const UChar gMinusX[] =                  {0x2D, 0x78, 0};    /* "-x" */
static const UChar gXDotX[] =                   {0x78, 0x2E, 0x78, 0}; /* "x.x" */
static const UChar gXDotZero[] =                {0x78, 0x2E, 0x30, 0}; /* "x.0" */
static const UChar gZeroDotX[] =                {0x30, 0x2E, 0x78, 0}; /* "0.x" */

static const UChar gLessLess[] =                {0x3C, 0x3C, 0};    /* "<<" */
static const UChar gLessPercent[] =             {0x3C, 0x25, 0};    /* "<%" */
static const UChar gLessHash[] =                {0x3C, 0x23, 0};    /* "<#" */
static const UChar gLessZero[] =                {0x3C, 0x30, 0};    /* "<0" */
static const UChar gGreaterGreater[] =          {0x3E, 0x3E, 0};    /* ">>" */
static const UChar gGreaterPercent[] =          {0x3E, 0x25, 0};    /* ">%" */
static const UChar gGreaterHash[] =             {0x3E, 0x23, 0};    /* ">#" */
static const UChar gGreaterZero[] =             {0x3E, 0x30, 0};    /* ">0" */
static const UChar gEqualPercent[] =            {0x3D, 0x25, 0};    /* "=%" */
static const UChar gEqualHash[] =               {0x3D, 0x23, 0};    /* "=#" */
static const UChar gEqualZero[] =               {0x3D, 0x30, 0};    /* "=0" */
static const UChar gEmptyString[] =             {0};                /* "" */
static const UChar gGreaterGreaterGreater[] =   {0x3E, 0x3E, 0x3E, 0}; /* ">>>" */

static const UChar * const tokenStrings[] = {
    gLessLess, gLessPercent, gLessHash, gLessZero,
    gGreaterGreater, gGreaterPercent,gGreaterHash, gGreaterZero,
    gEqualPercent, gEqualHash, gEqualZero, NULL
};

void
NFRule::makeRules(UnicodeString& description,
                  const NFRuleSet *ruleSet,
                  const NFRule *predecessor,
                  const RuleBasedNumberFormat *rbnf,
                  NFRuleList& rules,
                  UErrorCode& status)
{
    // we know we're making at least one rule, so go ahead and
    // new it up and initialize its basevalue and divisor
    // (this also strips the rule descriptor, if any, off the
    // descripton string)
    NFRule* rule1 = new NFRule(rbnf);
    /* test for NULL */
    if (rule1 == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    rule1->parseRuleDescriptor(description, status);

    // check the description to see whether there's text enclosed
    // in brackets
    int32_t brack1 = description.indexOf(gLeftBracket);
    int32_t brack2 = description.indexOf(gRightBracket);

    // if the description doesn't contain a matched pair of brackets,
    // or if it's of a type that doesn't recognize bracketed text,
    // then leave the description alone, initialize the rule's
    // rule text and substitutions, and return that rule
    if (brack1 == -1 || brack2 == -1 || brack1 > brack2
        || rule1->getType() == kProperFractionRule
        || rule1->getType() == kNegativeNumberRule) {
        rule1->ruleText = description;
        rule1->extractSubstitutions(ruleSet, predecessor, rbnf, status);
        rules.add(rule1);
    } else {
        // if the description does contain a matched pair of brackets,
        // then it's really shorthand for two rules (with one exception)
        NFRule* rule2 = NULL;
        UnicodeString sbuf;

        // we'll actually only split the rule into two rules if its
        // base value is an even multiple of its divisor (or it's one
        // of the special rules)
        if ((rule1->baseValue > 0
            && (rule1->baseValue % util64_pow(rule1->radix, rule1->exponent)) == 0)
            || rule1->getType() == kImproperFractionRule
            || rule1->getType() == kMasterRule) {

            // if it passes that test, new up the second rule.  If the
            // rule set both rules will belong to is a fraction rule
            // set, they both have the same base value; otherwise,
            // increment the original rule's base value ("rule1" actually
            // goes SECOND in the rule set's rule list)
            rule2 = new NFRule(rbnf);
            /* test for NULL */
            if (rule2 == 0) {
                status = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
            if (rule1->baseValue >= 0) {
                rule2->baseValue = rule1->baseValue;
                if (!ruleSet->isFractionRuleSet()) {
                    ++rule1->baseValue;
                }
            }

            // if the description began with "x.x" and contains bracketed
            // text, it describes both the improper fraction rule and
            // the proper fraction rule
            else if (rule1->getType() == kImproperFractionRule) {
                rule2->setType(kProperFractionRule);
            }

            // if the description began with "x.0" and contains bracketed
            // text, it describes both the master rule and the
            // improper fraction rule
            else if (rule1->getType() == kMasterRule) {
                rule2->baseValue = rule1->baseValue;
                rule1->setType(kImproperFractionRule);
            }

            // both rules have the same radix and exponent (i.e., the
            // same divisor)
            rule2->radix = rule1->radix;
            rule2->exponent = rule1->exponent;

            // rule2's rule text omits the stuff in brackets: initalize
            // its rule text and substitutions accordingly
            sbuf.append(description, 0, brack1);
            if (brack2 + 1 < description.length()) {
                sbuf.append(description, brack2 + 1, description.length() - brack2 - 1);
            }
            rule2->ruleText.setTo(sbuf);
            rule2->extractSubstitutions(ruleSet, predecessor, rbnf, status);
        }

        // rule1's text includes the text in the brackets but omits
        // the brackets themselves: initialize _its_ rule text and
        // substitutions accordingly
        sbuf.setTo(description, 0, brack1);
        sbuf.append(description, brack1 + 1, brack2 - brack1 - 1);
        if (brack2 + 1 < description.length()) {
            sbuf.append(description, brack2 + 1, description.length() - brack2 - 1);
        }
        rule1->ruleText.setTo(sbuf);
        rule1->extractSubstitutions(ruleSet, predecessor, rbnf, status);

        // if we only have one rule, return it; if we have two, return
        // a two-element array containing them (notice that rule2 goes
        // BEFORE rule1 in the list: in all cases, rule2 OMITS the
        // material in the brackets and rule1 INCLUDES the material
        // in the brackets)
        if (rule2 != NULL) {
            rules.add(rule2);
        }
        rules.add(rule1);
    }
}

/**
 * This function parses the rule's rule descriptor (i.e., the base
 * value and/or other tokens that precede the rule's rule text
 * in the description) and sets the rule's base value, radix, and
 * exponent according to the descriptor.  (If the description doesn't
 * include a rule descriptor, then this function sets everything to
 * default values and the rule set sets the rule's real base value).
 * @param description The rule's description
 * @return If "description" included a rule descriptor, this is
 * "description" with the descriptor and any trailing whitespace
 * stripped off.  Otherwise; it's "descriptor" unchangd.
 */
void
NFRule::parseRuleDescriptor(UnicodeString& description, UErrorCode& status)
{
    // the description consists of a rule descriptor and a rule body,
    // separated by a colon.  The rule descriptor is optional.  If
    // it's omitted, just set the base value to 0.
    int32_t p = description.indexOf(gColon);
    if (p == -1) {
        setBaseValue((int32_t)0, status);
    } else {
        // copy the descriptor out into its own string and strip it,
        // along with any trailing whitespace, out of the original
        // description
        UnicodeString descriptor;
        descriptor.setTo(description, 0, p);

        ++p;
        while (p < description.length() && uprv_isRuleWhiteSpace(description.charAt(p))) {
            ++p;
        }
        description.removeBetween(0, p);

        // check first to see if the rule descriptor matches the token
        // for one of the special rules.  If it does, set the base
        // value to the correct identfier value
        if (descriptor == gMinusX) {
            setType(kNegativeNumberRule);
        }
        else if (descriptor == gXDotX) {
            setType(kImproperFractionRule);
        }
        else if (descriptor == gZeroDotX) {
            setType(kProperFractionRule);
        }
        else if (descriptor == gXDotZero) {
            setType(kMasterRule);
        }

        // if the rule descriptor begins with a digit, it's a descriptor
        // for a normal rule
        // since we don't have Long.parseLong, and this isn't much work anyway,
        // just build up the value as we encounter the digits.
        else if (descriptor.charAt(0) >= gZero && descriptor.charAt(0) <= gNine) {
            int64_t val = 0;
            p = 0;
            UChar c = gSpace;

            // begin parsing the descriptor: copy digits
            // into "tempValue", skip periods, commas, and spaces,
            // stop on a slash or > sign (or at the end of the string),
            // and throw an exception on any other character
            int64_t ll_10 = 10;
            while (p < descriptor.length()) {
                c = descriptor.charAt(p);
                if (c >= gZero && c <= gNine) {
                    val = val * ll_10 + (int32_t)(c - gZero);
                }
                else if (c == gSlash || c == gGreaterThan) {
                    break;
                }
                else if (uprv_isRuleWhiteSpace(c) || c == gComma || c == gDot) {
                }
                else {
                    // throw new IllegalArgumentException("Illegal character in rule descriptor");
                    status = U_PARSE_ERROR;
                    return;
                }
                ++p;
            }

            // we have the base value, so set it
            setBaseValue(val, status);

            // if we stopped the previous loop on a slash, we're
            // now parsing the rule's radix.  Again, accumulate digits
            // in tempValue, skip punctuation, stop on a > mark, and
            // throw an exception on anything else
            if (c == gSlash) {
                val = 0;
                ++p;
                int64_t ll_10 = 10;
                while (p < descriptor.length()) {
                    c = descriptor.charAt(p);
                    if (c >= gZero && c <= gNine) {
                        val = val * ll_10 + (int32_t)(c - gZero);
                    }
                    else if (c == gGreaterThan) {
                        break;
                    }
                    else if (uprv_isRuleWhiteSpace(c) || c == gComma || c == gDot) {
                    }
                    else {
                        // throw new IllegalArgumentException("Illegal character is rule descriptor");
                        status = U_PARSE_ERROR;
                        return;
                    }
                    ++p;
                }

                // tempValue now contain's the rule's radix.  Set it
                // accordingly, and recalculate the rule's exponent
                radix = (int32_t)val;
                if (radix == 0) {
                    // throw new IllegalArgumentException("Rule can't have radix of 0");
                    status = U_PARSE_ERROR;
                }

                exponent = expectedExponent();
            }

            // if we stopped the previous loop on a > sign, then continue
            // for as long as we still see > signs.  For each one,
            // decrement the exponent (unless the exponent is already 0).
            // If we see another character before reaching the end of
            // the descriptor, that's also a syntax error.
            if (c == gGreaterThan) {
                while (p < descriptor.length()) {
                    c = descriptor.charAt(p);
                    if (c == gGreaterThan && exponent > 0) {
                        --exponent;
                    } else {
                        // throw new IllegalArgumentException("Illegal character in rule descriptor");
                        status = U_PARSE_ERROR;
                        return;
                    }
                    ++p;
                }
            }
        }
    }

    // finally, if the rule body begins with an apostrophe, strip it off
    // (this is generally used to put whitespace at the beginning of
    // a rule's rule text)
    if (description.length() > 0 && description.charAt(0) == gTick) {
        description.removeBetween(0, 1);
    }

    // return the description with all the stuff we've just waded through
    // stripped off the front.  It now contains just the rule body.
    // return description;
}

/**
* Searches the rule's rule text for the substitution tokens,
* creates the substitutions, and removes the substitution tokens
* from the rule's rule text.
* @param owner The rule set containing this rule
* @param predecessor The rule preseding this one in "owners" rule list
* @param ownersOwner The RuleBasedFormat that owns this rule
*/
void
NFRule::extractSubstitutions(const NFRuleSet* ruleSet,
                             const NFRule* predecessor,
                             const RuleBasedNumberFormat* rbnf,
                             UErrorCode& status)
{
    if (U_SUCCESS(status)) {
        sub1 = extractSubstitution(ruleSet, predecessor, rbnf, status);
        sub2 = extractSubstitution(ruleSet, predecessor, rbnf, status);
    }
}

/**
* Searches the rule's rule text for the first substitution token,
* creates a substitution based on it, and removes the token from
* the rule's rule text.
* @param owner The rule set containing this rule
* @param predecessor The rule preceding this one in the rule set's
* rule list
* @param ownersOwner The RuleBasedNumberFormat that owns this rule
* @return The newly-created substitution.  This is never null; if
* the rule text doesn't contain any substitution tokens, this will
* be a NullSubstitution.
*/
NFSubstitution *
NFRule::extractSubstitution(const NFRuleSet* ruleSet,
                            const NFRule* predecessor,
                            const RuleBasedNumberFormat* rbnf,
                            UErrorCode& status)
{
    NFSubstitution* result = NULL;

    // search the rule's rule text for the first two characters of
    // a substitution token
    int32_t subStart = indexOfAny(tokenStrings);
    int32_t subEnd = subStart;

    // if we didn't find one, create a null substitution positioned
    // at the end of the rule text
    if (subStart == -1) {
        return NFSubstitution::makeSubstitution(ruleText.length(), this, predecessor,
            ruleSet, rbnf, gEmptyString, status);
    }

    // special-case the ">>>" token, since searching for the > at the
    // end will actually find the > in the middle
    if (ruleText.indexOf(gGreaterGreaterGreater) == subStart) {
        subEnd = subStart + 2;

        // otherwise the substitution token ends with the same character
        // it began with
    } else {
        UChar c = ruleText.charAt(subStart);
        subEnd = ruleText.indexOf(c, subStart + 1);
        // special case for '<%foo<<'
        if (c == gLessThan && subEnd != -1 && subEnd < ruleText.length() - 1 && ruleText.charAt(subEnd+1) == c) {
            // ordinals use "=#,##0==%abbrev=" as their rule.  Notice that the '==' in the middle
            // occurs because of the juxtaposition of two different rules.  The check for '<' is a hack
            // to get around this.  Having the duplicate at the front would cause problems with
            // rules like "<<%" to format, say, percents...
            ++subEnd;
        }
   }

    // if we don't find the end of the token (i.e., if we're on a single,
    // unmatched token character), create a null substitution positioned
    // at the end of the rule
    if (subEnd == -1) {
        return NFSubstitution::makeSubstitution(ruleText.length(), this, predecessor,
            ruleSet, rbnf, gEmptyString, status);
    }

    // if we get here, we have a real substitution token (or at least
    // some text bounded by substitution token characters).  Use
    // makeSubstitution() to create the right kind of substitution
    UnicodeString subToken;
    subToken.setTo(ruleText, subStart, subEnd + 1 - subStart);
    result = NFSubstitution::makeSubstitution(subStart, this, predecessor, ruleSet,
        rbnf, subToken, status);

    // remove the substitution from the rule text
    ruleText.removeBetween(subStart, subEnd+1);

    return result;
}

/**
 * Sets the rule's base value, and causes the radix and exponent
 * to be recalculated.  This is used during construction when we
 * don't know the rule's base value until after it's been
 * constructed.  It should be used at any other time.
 * @param The new base value for the rule.
 */
void
NFRule::setBaseValue(int64_t newBaseValue, UErrorCode& status)
{
    // set the base value
    baseValue = newBaseValue;

    // if this isn't a special rule, recalculate the radix and exponent
    // (the radix always defaults to 10; if it's supposed to be something
    // else, it's cleaned up by the caller and the exponent is
    // recalculated again-- the only function that does this is
    // NFRule.parseRuleDescriptor() )
    if (baseValue >= 1) {
        radix = 10;
        exponent = expectedExponent();

        // this function gets called on a fully-constructed rule whose
        // description didn't specify a base value.  This means it
        // has substitutions, and some substitutions hold on to copies
        // of the rule's divisor.  Fix their copies of the divisor.
        if (sub1 != NULL) {
            sub1->setDivisor(radix, exponent, status);
        }
        if (sub2 != NULL) {
            sub2->setDivisor(radix, exponent, status);
        }

        // if this is a special rule, its radix and exponent are basically
        // ignored.  Set them to "safe" default values
    } else {
        radix = 10;
        exponent = 0;
    }
}

/**
* This calculates the rule's exponent based on its radix and base
* value.  This will be the highest power the radix can be raised to
* and still produce a result less than or equal to the base value.
*/
int16_t
NFRule::expectedExponent() const
{
    // since the log of 0, or the log base 0 of something, causes an
    // error, declare the exponent in these cases to be 0 (we also
    // deal with the special-rule identifiers here)
    if (radix == 0 || baseValue < 1) {
        return 0;
    }

    // we get rounding error in some cases-- for example, log 1000 / log 10
    // gives us 1.9999999996 instead of 2.  The extra logic here is to take
    // that into account
    int16_t tempResult = (int16_t)(uprv_log((double)baseValue) / uprv_log((double)radix));
    int64_t temp = util64_pow(radix, tempResult + 1);
    if (temp <= baseValue) {
        tempResult += 1;
    }
    return tempResult;
}

/**
 * Searches the rule's rule text for any of the specified strings.
 * @param strings An array of strings to search the rule's rule
 * text for
 * @return The index of the first match in the rule's rule text
 * (i.e., the first substring in the rule's rule text that matches
 * _any_ of the strings in "strings").  If none of the strings in
 * "strings" is found in the rule's rule text, returns -1.
 */
int32_t
NFRule::indexOfAny(const UChar* const strings[]) const
{
    int result = -1;
    for (int i = 0; strings[i]; i++) {
        int32_t pos = ruleText.indexOf(*strings[i]);
        if (pos != -1 && (result == -1 || pos < result)) {
            result = pos;
        }
    }
    return result;
}

//-----------------------------------------------------------------------
// boilerplate
//-----------------------------------------------------------------------

/**
* Tests two rules for equality.
* @param that The rule to compare this one against
* @return True is the two rules are functionally equivalent
*/
UBool
NFRule::operator==(const NFRule& rhs) const
{
    return baseValue == rhs.baseValue
        && radix == rhs.radix
        && exponent == rhs.exponent
        && ruleText == rhs.ruleText
        && *sub1 == *rhs.sub1
        && *sub2 == *rhs.sub2;
}

/**
* Returns a textual representation of the rule.  This won't
* necessarily be the same as the description that this rule
* was created with, but it will produce the same result.
* @return A textual description of the rule
*/
static void util_append64(UnicodeString& result, int64_t n)
{
    UChar buffer[256];
    int32_t len = util64_tou(n, buffer, sizeof(buffer));
    UnicodeString temp(buffer, len);
    result.append(temp);
}

void
NFRule::_appendRuleText(UnicodeString& result) const
{
    switch (getType()) {
    case kNegativeNumberRule: result.append(gMinusX); break;
    case kImproperFractionRule: result.append(gXDotX); break;
    case kProperFractionRule: result.append(gZeroDotX); break;
    case kMasterRule: result.append(gXDotZero); break;
    default:
        // for a normal rule, write out its base value, and if the radix is
        // something other than 10, write out the radix (with the preceding
        // slash, of course).  Then calculate the expected exponent and if
        // if isn't the same as the actual exponent, write an appropriate
        // number of > signs.  Finally, terminate the whole thing with
        // a colon.
        util_append64(result, baseValue);
        if (radix != 10) {
            result.append(gSlash);
            util_append64(result, radix);
        }
        int numCarets = expectedExponent() - exponent;
        for (int i = 0; i < numCarets; i++) {
            result.append(gGreaterThan);
        }
        break;
    }
    result.append(gColon);
    result.append(gSpace);

    // if the rule text begins with a space, write an apostrophe
    // (whitespace after the rule descriptor is ignored; the
    // apostrophe is used to make the whitespace significant)
    if (ruleText.startsWith(gSpace) && sub1->getPos() != 0) {
        result.append(gTick);
    }

    // now, write the rule's rule text, inserting appropriate
    // substitution tokens in the appropriate places
    UnicodeString ruleTextCopy;
    ruleTextCopy.setTo(ruleText);

    UnicodeString temp;
    sub2->toString(temp);
    ruleTextCopy.insert(sub2->getPos(), temp);
    sub1->toString(temp);
    ruleTextCopy.insert(sub1->getPos(), temp);

    result.append(ruleTextCopy);

    // and finally, top the whole thing off with a semicolon and
    // return the result
    result.append(gSemicolon);
}

//-----------------------------------------------------------------------
// formatting
//-----------------------------------------------------------------------

/**
* Formats the number, and inserts the resulting text into
* toInsertInto.
* @param number The number being formatted
* @param toInsertInto The string where the resultant text should
* be inserted
* @param pos The position in toInsertInto where the resultant text
* should be inserted
*/
void
NFRule::doFormat(int64_t number, UnicodeString& toInsertInto, int32_t pos) const
{
    // first, insert the rule's rule text into toInsertInto at the
    // specified position, then insert the results of the substitutions
    // into the right places in toInsertInto (notice we do the
    // substitutions in reverse order so that the offsets don't get
    // messed up)
    toInsertInto.insert(pos, ruleText);
    sub2->doSubstitution(number, toInsertInto, pos);
    sub1->doSubstitution(number, toInsertInto, pos);
}

/**
* Formats the number, and inserts the resulting text into
* toInsertInto.
* @param number The number being formatted
* @param toInsertInto The string where the resultant text should
* be inserted
* @param pos The position in toInsertInto where the resultant text
* should be inserted
*/
void
NFRule::doFormat(double number, UnicodeString& toInsertInto, int32_t pos) const
{
    // first, insert the rule's rule text into toInsertInto at the
    // specified position, then insert the results of the substitutions
    // into the right places in toInsertInto
    // [again, we have two copies of this routine that do the same thing
    // so that we don't sacrifice precision in a long by casting it
    // to a double]
    toInsertInto.insert(pos, ruleText);
    sub2->doSubstitution(number, toInsertInto, pos);
    sub1->doSubstitution(number, toInsertInto, pos);
}

/**
* Used by the owning rule set to determine whether to invoke the
* rollback rule (i.e., whether this rule or the one that precedes
* it in the rule set's list should be used to format the number)
* @param The number being formatted
* @return True if the rule set should use the rule that precedes
* this one in its list; false if it should use this rule
*/
UBool
NFRule::shouldRollBack(double number) const
{
    // we roll back if the rule contains a modulus substitution,
    // the number being formatted is an even multiple of the rule's
    // divisor, and the rule's base value is NOT an even multiple
    // of its divisor
    // In other words, if the original description had
    //    100: << hundred[ >>];
    // that expands into
    //    100: << hundred;
    //    101: << hundred >>;
    // internally.  But when we're formatting 200, if we use the rule
    // at 101, which would normally apply, we get "two hundred zero".
    // To prevent this, we roll back and use the rule at 100 instead.
    // This is the logic that makes this happen: the rule at 101 has
    // a modulus substitution, its base value isn't an even multiple
    // of 100, and the value we're trying to format _is_ an even
    // multiple of 100.  This is called the "rollback rule."
    if ((sub1->isModulusSubstitution()) || (sub2->isModulusSubstitution())) {
        int64_t re = util64_pow(radix, exponent);
        return uprv_fmod(number, (double)re) == 0 && (baseValue % re) != 0;
    }
    return FALSE;
}

//-----------------------------------------------------------------------
// parsing
//-----------------------------------------------------------------------

/**
* Attempts to parse the string with this rule.
* @param text The string being parsed
* @param parsePosition On entry, the value is ignored and assumed to
* be 0. On exit, this has been updated with the position of the first
* character not consumed by matching the text against this rule
* (if this rule doesn't match the text at all, the parse position
* if left unchanged (presumably at 0) and the function returns
* new Long(0)).
* @param isFractionRule True if this rule is contained within a
* fraction rule set.  This is only used if the rule has no
* substitutions.
* @return If this rule matched the text, this is the rule's base value
* combined appropriately with the results of parsing the substitutions.
* If nothing matched, this is new Long(0) and the parse position is
* left unchanged.  The result will be an instance of Long if the
* result is an integer and Double otherwise.  The result is never null.
*/
#ifdef RBNF_DEBUG
#include <stdio.h>

static void dumpUS(FILE* f, const UnicodeString& us) {
  int len = us.length();
  char* buf = (char *)uprv_malloc((len+1)*sizeof(char)); //new char[len+1];
  if (buf != NULL) {
	  us.extract(0, len, buf);
	  buf[len] = 0;
	  fprintf(f, "%s", buf);
	  uprv_free(buf); //delete[] buf;
  }
}
#endif

UBool
NFRule::doParse(const UnicodeString& text,
                ParsePosition& parsePosition,
                UBool isFractionRule,
                double upperBound,
                Formattable& resVal) const
{
    // internally we operate on a copy of the string being parsed
    // (because we're going to change it) and use our own ParsePosition
    ParsePosition pp;
    UnicodeString workText(text);

    // check to see whether the text before the first substitution
    // matches the text at the beginning of the string being
    // parsed.  If it does, strip that off the front of workText;
    // otherwise, dump out with a mismatch
    UnicodeString prefix;
    prefix.setTo(ruleText, 0, sub1->getPos());

#ifdef RBNF_DEBUG
    fprintf(stderr, "doParse %x ", this);
    {
        UnicodeString rt;
        _appendRuleText(rt);
        dumpUS(stderr, rt);
    }

    fprintf(stderr, " text: '", this);
    dumpUS(stderr, text);
    fprintf(stderr, "' prefix: '");
    dumpUS(stderr, prefix);
#endif
    stripPrefix(workText, prefix, pp);
    int32_t prefixLength = text.length() - workText.length();

#ifdef RBNF_DEBUG
    fprintf(stderr, "' pl: %d ppi: %d s1p: %d\n", prefixLength, pp.getIndex(), sub1->getPos());
#endif

    if (pp.getIndex() == 0 && sub1->getPos() != 0) {
        // commented out because ParsePosition doesn't have error index in 1.1.x
        // restored for ICU4C port
        parsePosition.setErrorIndex(pp.getErrorIndex());
        resVal.setLong(0);
        return TRUE;
    }

    // this is the fun part.  The basic guts of the rule-matching
    // logic is matchToDelimiter(), which is called twice.  The first
    // time it searches the input string for the rule text BETWEEN
    // the substitutions and tries to match the intervening text
    // in the input string with the first substitution.  If that
    // succeeds, it then calls it again, this time to look for the
    // rule text after the second substitution and to match the
    // intervening input text against the second substitution.
    //
    // For example, say we have a rule that looks like this:
    //    first << middle >> last;
    // and input text that looks like this:
    //    first one middle two last
    // First we use stripPrefix() to match "first " in both places and
    // strip it off the front, leaving
    //    one middle two last
    // Then we use matchToDelimiter() to match " middle " and try to
    // match "one" against a substitution.  If it's successful, we now
    // have
    //    two last
    // We use matchToDelimiter() a second time to match " last" and
    // try to match "two" against a substitution.  If "two" matches
    // the substitution, we have a successful parse.
    //
    // Since it's possible in many cases to find multiple instances
    // of each of these pieces of rule text in the input string,
    // we need to try all the possible combinations of these
    // locations.  This prevents us from prematurely declaring a mismatch,
    // and makes sure we match as much input text as we can.
    int highWaterMark = 0;
    double result = 0;
    int start = 0;
    double tempBaseValue = (double)(baseValue <= 0 ? 0 : baseValue);

    UnicodeString temp;
    do {
        // our partial parse result starts out as this rule's base
        // value.  If it finds a successful match, matchToDelimiter()
        // will compose this in some way with what it gets back from
        // the substitution, giving us a new partial parse result
        pp.setIndex(0);

        temp.setTo(ruleText, sub1->getPos(), sub2->getPos() - sub1->getPos());
        double partialResult = matchToDelimiter(workText, start, tempBaseValue,
            temp, pp, sub1,
            upperBound);

        // if we got a successful match (or were trying to match a
        // null substitution), pp is now pointing at the first unmatched
        // character.  Take note of that, and try matchToDelimiter()
        // on the input text again
        if (pp.getIndex() != 0 || sub1->isNullSubstitution()) {
            start = pp.getIndex();

            UnicodeString workText2;
            workText2.setTo(workText, pp.getIndex(), workText.length() - pp.getIndex());
            ParsePosition pp2;

            // the second matchToDelimiter() will compose our previous
            // partial result with whatever it gets back from its
            // substitution if there's a successful match, giving us
            // a real result
            temp.setTo(ruleText, sub2->getPos(), ruleText.length() - sub2->getPos());
            partialResult = matchToDelimiter(workText2, 0, partialResult,
                temp, pp2, sub2,
                upperBound);

            // if we got a successful match on this second
            // matchToDelimiter() call, update the high-water mark
            // and result (if necessary)
            if (pp2.getIndex() != 0 || sub2->isNullSubstitution()) {
                if (prefixLength + pp.getIndex() + pp2.getIndex() > highWaterMark) {
                    highWaterMark = prefixLength + pp.getIndex() + pp2.getIndex();
                    result = partialResult;
                }
            }
            // commented out because ParsePosition doesn't have error index in 1.1.x
            // restored for ICU4C port
            else {
                int32_t temp = pp2.getErrorIndex() + sub1->getPos() + pp.getIndex();
                if (temp> parsePosition.getErrorIndex()) {
                    parsePosition.setErrorIndex(temp);
                }
            }
        }
        // commented out because ParsePosition doesn't have error index in 1.1.x
        // restored for ICU4C port
        else {
            int32_t temp = sub1->getPos() + pp.getErrorIndex();
            if (temp > parsePosition.getErrorIndex()) {
                parsePosition.setErrorIndex(temp);
            }
        }
        // keep trying to match things until the outer matchToDelimiter()
        // call fails to make a match (each time, it picks up where it
        // left off the previous time)
    } while (sub1->getPos() != sub2->getPos()
        && pp.getIndex() > 0
        && pp.getIndex() < workText.length()
        && pp.getIndex() != start);

    // update the caller's ParsePosition with our high-water mark
    // (i.e., it now points at the first character this function
    // didn't match-- the ParsePosition is therefore unchanged if
    // we didn't match anything)
    parsePosition.setIndex(highWaterMark);
    // commented out because ParsePosition doesn't have error index in 1.1.x
    // restored for ICU4C port
    if (highWaterMark > 0) {
        parsePosition.setErrorIndex(0);
    }

    // this is a hack for one unusual condition: Normally, whether this
    // rule belong to a fraction rule set or not is handled by its
    // substitutions.  But if that rule HAS NO substitutions, then
    // we have to account for it here.  By definition, if the matching
    // rule in a fraction rule set has no substitutions, its numerator
    // is 1, and so the result is the reciprocal of its base value.
    if (isFractionRule &&
        highWaterMark > 0 &&
        sub1->isNullSubstitution()) {
        result = 1 / result;
    }

    resVal.setDouble(result);
    return TRUE; // ??? do we need to worry if it is a long or a double?
}

/**
* This function is used by parse() to match the text being parsed
* against a possible prefix string.  This function
* matches characters from the beginning of the string being parsed
* to characters from the prospective prefix.  If they match, pp is
* updated to the first character not matched, and the result is
* the unparsed part of the string.  If they don't match, the whole
* string is returned, and pp is left unchanged.
* @param text The string being parsed
* @param prefix The text to match against
* @param pp On entry, ignored and assumed to be 0.  On exit, points
* to the first unmatched character (assuming the whole prefix matched),
* or is unchanged (if the whole prefix didn't match).
* @return If things match, this is the unparsed part of "text";
* if they didn't match, this is "text".
*/
void
NFRule::stripPrefix(UnicodeString& text, const UnicodeString& prefix, ParsePosition& pp) const
{
    // if the prefix text is empty, dump out without doing anything
    if (prefix.length() != 0) {
    	UErrorCode status = U_ZERO_ERROR;
        // use prefixLength() to match the beginning of
        // "text" against "prefix".  This function returns the
        // number of characters from "text" that matched (or 0 if
        // we didn't match the whole prefix)
        int32_t pfl = prefixLength(text, prefix, status);
        if (U_FAILURE(status)) { // Memory allocation error.
        	return;
        }
        if (pfl != 0) {
            // if we got a successful match, update the parse position
            // and strip the prefix off of "text"
            pp.setIndex(pp.getIndex() + pfl);
            text.remove(0, pfl);
        }
    }
}

/**
* Used by parse() to match a substitution and any following text.
* "text" is searched for instances of "delimiter".  For each instance
* of delimiter, the intervening text is tested to see whether it
* matches the substitution.  The longest match wins.
* @param text The string being parsed
* @param startPos The position in "text" where we should start looking
* for "delimiter".
* @param baseValue A partial parse result (often the rule's base value),
* which is combined with the result from matching the substitution
* @param delimiter The string to search "text" for.
* @param pp Ignored and presumed to be 0 on entry.  If there's a match,
* on exit this will point to the first unmatched character.
* @param sub If we find "delimiter" in "text", this substitution is used
* to match the text between the beginning of the string and the
* position of "delimiter."  (If "delimiter" is the empty string, then
* this function just matches against this substitution and updates
* everything accordingly.)
* @param upperBound When matching the substitution, it will only
* consider rules with base values lower than this value.
* @return If there's a match, this is the result of composing
* baseValue with the result of matching the substitution.  Otherwise,
* this is new Long(0).  It's never null.  If the result is an integer,
* this will be an instance of Long; otherwise, it's an instance of
* Double.
*
* !!! note {dlf} in point of fact, in the java code the caller always converts
* the result to a double, so we might as well return one.
*/
double
NFRule::matchToDelimiter(const UnicodeString& text,
                         int32_t startPos,
                         double _baseValue,
                         const UnicodeString& delimiter,
                         ParsePosition& pp,
                         const NFSubstitution* sub,
                         double upperBound) const
{
	UErrorCode status = U_ZERO_ERROR;
    // if "delimiter" contains real (i.e., non-ignorable) text, search
    // it for "delimiter" beginning at "start".  If that succeeds, then
    // use "sub"'s doParse() method to match the text before the
    // instance of "delimiter" we just found.
    if (!allIgnorable(delimiter, status)) {
    	if (U_FAILURE(status)) { //Memory allocation error.
    		return 0;
    	}
        ParsePosition tempPP;
        Formattable result;

        // use findText() to search for "delimiter".  It returns a two-
        // element array: element 0 is the position of the match, and
        // element 1 is the number of characters that matched
        // "delimiter".
        int32_t dLen;
        int32_t dPos = findText(text, delimiter, startPos, &dLen);

        // if findText() succeeded, isolate the text preceding the
        // match, and use "sub" to match that text
        while (dPos >= 0) {
            UnicodeString subText;
            subText.setTo(text, 0, dPos);
            if (subText.length() > 0) {
                UBool success = sub->doParse(subText, tempPP, _baseValue, upperBound,
#if UCONFIG_NO_COLLATION
                    FALSE,
#else
                    formatter->isLenient(),
#endif
                    result);

                // if the substitution could match all the text up to
                // where we found "delimiter", then this function has
                // a successful match.  Bump the caller's parse position
                // to point to the first character after the text
                // that matches "delimiter", and return the result
                // we got from parsing the substitution.
                if (success && tempPP.getIndex() == dPos) {
                    pp.setIndex(dPos + dLen);
                    return result.getDouble();
                }
                // commented out because ParsePosition doesn't have error index in 1.1.x
                // restored for ICU4C port
                else {
                    if (tempPP.getErrorIndex() > 0) {
                        pp.setErrorIndex(tempPP.getErrorIndex());
                    } else {
                        pp.setErrorIndex(tempPP.getIndex());
                    }
                }
            }

            // if we didn't match the substitution, search for another
            // copy of "delimiter" in "text" and repeat the loop if
            // we find it
            tempPP.setIndex(0);
            dPos = findText(text, delimiter, dPos + dLen, &dLen);
        }
        // if we make it here, this was an unsuccessful match, and we
        // leave pp unchanged and return 0
        pp.setIndex(0);
        return 0;

        // if "delimiter" is empty, or consists only of ignorable characters
        // (i.e., is semantically empty), thwe we obviously can't search
        // for "delimiter".  Instead, just use "sub" to parse as much of
        // "text" as possible.
    } else {
        ParsePosition tempPP;
        Formattable result;

        // try to match the whole string against the substitution
        UBool success = sub->doParse(text, tempPP, _baseValue, upperBound,
#if UCONFIG_NO_COLLATION
            FALSE,
#else
            formatter->isLenient(),
#endif
            result);
        if (success && (tempPP.getIndex() != 0 || sub->isNullSubstitution())) {
            // if there's a successful match (or it's a null
            // substitution), update pp to point to the first
            // character we didn't match, and pass the result from
            // sub.doParse() on through to the caller
            pp.setIndex(tempPP.getIndex());
            return result.getDouble();
        }
        // commented out because ParsePosition doesn't have error index in 1.1.x
        // restored for ICU4C port
        else {
            pp.setErrorIndex(tempPP.getErrorIndex());
        }

        // and if we get to here, then nothing matched, so we return
        // 0 and leave pp alone
        return 0;
    }
}

/**
* Used by stripPrefix() to match characters.  If lenient parse mode
* is off, this just calls startsWith().  If lenient parse mode is on,
* this function uses CollationElementIterators to match characters in
* the strings (only primary-order differences are significant in
* determining whether there's a match).
* @param str The string being tested
* @param prefix The text we're hoping to see at the beginning
* of "str"
* @return If "prefix" is found at the beginning of "str", this
* is the number of characters in "str" that were matched (this
* isn't necessarily the same as the length of "prefix" when matching
* text with a collator).  If there's no match, this is 0.
*/
int32_t
NFRule::prefixLength(const UnicodeString& str, const UnicodeString& prefix, UErrorCode& status) const
{
    // if we're looking for an empty prefix, it obviously matches
    // zero characters.  Just go ahead and return 0.
    if (prefix.length() == 0) {
        return 0;
    }

#if !UCONFIG_NO_COLLATION
    // go through all this grief if we're in lenient-parse mode
    if (formatter->isLenient()) {
        // get the formatter's collator and use it to create two
        // collation element iterators, one over the target string
        // and another over the prefix (right now, we'll throw an
        // exception if the collator we get back from the formatter
        // isn't a RuleBasedCollator, because RuleBasedCollator defines
        // the CollationElementIterator protocol.  Hopefully, this
        // will change someday.)
        RuleBasedCollator* collator = (RuleBasedCollator*)formatter->getCollator();
        CollationElementIterator* strIter = collator->createCollationElementIterator(str);
        CollationElementIterator* prefixIter = collator->createCollationElementIterator(prefix);
        // Check for memory allocation error.
        if (collator == NULL || strIter == NULL || prefixIter == NULL) {
        	delete collator;
        	delete strIter;
        	delete prefixIter;
        	status = U_MEMORY_ALLOCATION_ERROR;
        	return 0;
        }

        UErrorCode err = U_ZERO_ERROR;

        // The original code was problematic.  Consider this match:
        // prefix = "fifty-"
        // string = " fifty-7"
        // The intent is to match string up to the '7', by matching 'fifty-' at position 1
        // in the string.  Unfortunately, we were getting a match, and then computing where
        // the match terminated by rematching the string.  The rematch code was using as an
        // initial guess the substring of string between 0 and prefix.length.  Because of
        // the leading space and trailing hyphen (both ignorable) this was succeeding, leaving
        // the position before the hyphen in the string.  Recursing down, we then parsed the
        // remaining string '-7' as numeric.  The resulting number turned out as 43 (50 - 7).
        // This was not pretty, especially since the string "fifty-7" parsed just fine.
        //
        // We have newer APIs now, so we can use calls on the iterator to determine what we
        // matched up to.  If we terminate because we hit the last element in the string,
        // our match terminates at this length.  If we terminate because we hit the last element
        // in the target, our match terminates at one before the element iterator position.

        // match collation elements between the strings
        int32_t oStr = strIter->next(err);
        int32_t oPrefix = prefixIter->next(err);

        while (oPrefix != CollationElementIterator::NULLORDER) {
            // skip over ignorable characters in the target string
            while (CollationElementIterator::primaryOrder(oStr) == 0
                && oStr != CollationElementIterator::NULLORDER) {
                oStr = strIter->next(err);
            }

            // skip over ignorable characters in the prefix
            while (CollationElementIterator::primaryOrder(oPrefix) == 0
                && oPrefix != CollationElementIterator::NULLORDER) {
                oPrefix = prefixIter->next(err);
            }

            // dlf: move this above following test, if we consume the
            // entire target, aren't we ok even if the source was also
            // entirely consumed?

            // if skipping over ignorables brought to the end of
            // the prefix, we DID match: drop out of the loop
            if (oPrefix == CollationElementIterator::NULLORDER) {
                break;
            }

            // if skipping over ignorables brought us to the end
            // of the target string, we didn't match and return 0
            if (oStr == CollationElementIterator::NULLORDER) {
                delete prefixIter;
                delete strIter;
                return 0;
            }

            // match collation elements from the two strings
            // (considering only primary differences).  If we
            // get a mismatch, dump out and return 0
            if (CollationElementIterator::primaryOrder(oStr)
                != CollationElementIterator::primaryOrder(oPrefix)) {
                delete prefixIter;
                delete strIter;
                return 0;

                // otherwise, advance to the next character in each string
                // and loop (we drop out of the loop when we exhaust
                // collation elements in the prefix)
            } else {
                oStr = strIter->next(err);
                oPrefix = prefixIter->next(err);
            }
        }

        int32_t result = strIter->getOffset();
        if (oStr != CollationElementIterator::NULLORDER) {
            --result; // back over character that we don't want to consume;
        }

#ifdef RBNF_DEBUG
        fprintf(stderr, "prefix length: %d\n", result);
#endif
        delete prefixIter;
        delete strIter;

        return result;
#if 0
        //----------------------------------------------------------------
        // JDK 1.2-specific API call
        // return strIter.getOffset();
        //----------------------------------------------------------------
        // JDK 1.1 HACK (take out for 1.2-specific code)

        // if we make it to here, we have a successful match.  Now we
        // have to find out HOW MANY characters from the target string
        // matched the prefix (there isn't necessarily a one-to-one
        // mapping between collation elements and characters).
        // In JDK 1.2, there's a simple getOffset() call we can use.
        // In JDK 1.1, on the other hand, we have to go through some
        // ugly contortions.  First, use the collator to compare the
        // same number of characters from the prefix and target string.
        // If they're equal, we're done.
        collator->setStrength(Collator::PRIMARY);
        if (str.length() >= prefix.length()) {
            UnicodeString temp;
            temp.setTo(str, 0, prefix.length());
            if (collator->equals(temp, prefix)) {
#ifdef RBNF_DEBUG
                fprintf(stderr, "returning: %d\n", prefix.length());
#endif
                return prefix.length();
            }
        }

        // if they're not equal, then we have to compare successively
        // larger and larger substrings of the target string until we
        // get to one that matches the prefix.  At that point, we know
        // how many characters matched the prefix, and we can return.
        int32_t p = 1;
        while (p <= str.length()) {
            UnicodeString temp;
            temp.setTo(str, 0, p);
            if (collator->equals(temp, prefix)) {
                return p;
            } else {
                ++p;
            }
        }

        // SHOULD NEVER GET HERE!!!
        return 0;
        //----------------------------------------------------------------
#endif

        // If lenient parsing is turned off, forget all that crap above.
        // Just use String.startsWith() and be done with it.
  } else
#endif
  {
      if (str.startsWith(prefix)) {
          return prefix.length();
      } else {
          return 0;
      }
  }
}

/**
* Searches a string for another string.  If lenient parsing is off,
* this just calls indexOf().  If lenient parsing is on, this function
* uses CollationElementIterator to match characters, and only
* primary-order differences are significant in determining whether
* there's a match.
* @param str The string to search
* @param key The string to search "str" for
* @param startingAt The index into "str" where the search is to
* begin
* @return A two-element array of ints.  Element 0 is the position
* of the match, or -1 if there was no match.  Element 1 is the
* number of characters in "str" that matched (which isn't necessarily
* the same as the length of "key")
*/
int32_t
NFRule::findText(const UnicodeString& str,
                 const UnicodeString& key,
                 int32_t startingAt,
                 int32_t* length) const
{
#if !UCONFIG_NO_COLLATION
    // if lenient parsing is turned off, this is easy: just call
    // String.indexOf() and we're done
    if (!formatter->isLenient()) {
        *length = key.length();
        return str.indexOf(key, startingAt);

        // but if lenient parsing is turned ON, we've got some work
        // ahead of us
    } else
#endif
    {
        //----------------------------------------------------------------
        // JDK 1.1 HACK (take out of 1.2-specific code)

        // in JDK 1.2, CollationElementIterator provides us with an
        // API to map between character offsets and collation elements
        // and we can do this by marching through the string comparing
        // collation elements.  We can't do that in JDK 1.1.  Insted,
        // we have to go through this horrible slow mess:
        int32_t p = startingAt;
        int32_t keyLen = 0;

        // basically just isolate smaller and smaller substrings of
        // the target string (each running to the end of the string,
        // and with the first one running from startingAt to the end)
        // and then use prefixLength() to see if the search key is at
        // the beginning of each substring.  This is excruciatingly
        // slow, but it will locate the key and tell use how long the
        // matching text was.
        UnicodeString temp;
        UErrorCode status = U_ZERO_ERROR;
        while (p < str.length() && keyLen == 0) {
            temp.setTo(str, p, str.length() - p);
            keyLen = prefixLength(temp, key, status);
            if (U_FAILURE(status)) {
            	break;
            }
            if (keyLen != 0) {
                *length = keyLen;
                return p;
            }
            ++p;
        }
        // if we make it to here, we didn't find it.  Return -1 for the
        // location.  The length should be ignored, but set it to 0,
        // which should be "safe"
        *length = 0;
        return -1;

        //----------------------------------------------------------------
        // JDK 1.2 version of this routine
        //RuleBasedCollator collator = (RuleBasedCollator)formatter.getCollator();
        //
        //CollationElementIterator strIter = collator.getCollationElementIterator(str);
        //CollationElementIterator keyIter = collator.getCollationElementIterator(key);
        //
        //int keyStart = -1;
        //
        //str.setOffset(startingAt);
        //
        //int oStr = strIter.next();
        //int oKey = keyIter.next();
        //while (oKey != CollationElementIterator.NULLORDER) {
        //    while (oStr != CollationElementIterator.NULLORDER &&
        //                CollationElementIterator.primaryOrder(oStr) == 0)
        //        oStr = strIter.next();
        //
        //    while (oKey != CollationElementIterator.NULLORDER &&
        //                CollationElementIterator.primaryOrder(oKey) == 0)
        //        oKey = keyIter.next();
        //
        //    if (oStr == CollationElementIterator.NULLORDER) {
        //        return new int[] { -1, 0 };
        //    }
        //
        //    if (oKey == CollationElementIterator.NULLORDER) {
        //        break;
        //    }
        //
        //    if (CollationElementIterator.primaryOrder(oStr) ==
        //            CollationElementIterator.primaryOrder(oKey)) {
        //        keyStart = strIter.getOffset();
        //        oStr = strIter.next();
        //        oKey = keyIter.next();
        //    } else {
        //        if (keyStart != -1) {
        //            keyStart = -1;
        //            keyIter.reset();
        //        } else {
        //            oStr = strIter.next();
        //        }
        //    }
        //}
        //
        //if (oKey == CollationElementIterator.NULLORDER) {
        //    return new int[] { keyStart, strIter.getOffset() - keyStart };
        //} else {
        //    return new int[] { -1, 0 };
        //}
    }
}

/**
* Checks to see whether a string consists entirely of ignorable
* characters.
* @param str The string to test.
* @return true if the string is empty of consists entirely of
* characters that the number formatter's collator says are
* ignorable at the primary-order level.  false otherwise.
*/
UBool
NFRule::allIgnorable(const UnicodeString& str, UErrorCode& status) const
{
    // if the string is empty, we can just return true
    if (str.length() == 0) {
        return TRUE;
    }

#if !UCONFIG_NO_COLLATION
    // if lenient parsing is turned on, walk through the string with
    // a collation element iterator and make sure each collation
    // element is 0 (ignorable) at the primary level
    if (formatter->isLenient()) {
        RuleBasedCollator* collator = (RuleBasedCollator*)(formatter->getCollator());
        CollationElementIterator* iter = collator->createCollationElementIterator(str);
        
        // Memory allocation error check.
        if (collator == NULL || iter == NULL) {
        	delete collator;
        	delete iter;
        	status = U_MEMORY_ALLOCATION_ERROR;
        	return FALSE;
        }

        UErrorCode err = U_ZERO_ERROR;
        int32_t o = iter->next(err);
        while (o != CollationElementIterator::NULLORDER
            && CollationElementIterator::primaryOrder(o) == 0) {
            o = iter->next(err);
        }

        delete iter;
        return o == CollationElementIterator::NULLORDER;
    }
#endif

    // if lenient parsing is turned off, there is no such thing as
    // an ignorable character: return true only if the string is empty
    return FALSE;
}

U_NAMESPACE_END

/* U_HAVE_RBNF */
#endif


