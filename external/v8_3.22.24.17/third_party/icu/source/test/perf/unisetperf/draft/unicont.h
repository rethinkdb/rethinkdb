/*  
**********************************************************************
*   Copyright (C) 2007, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   file name:  unicont.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2007jan15
*   created by: Markus Scherer
*
*   Idea for new common interface underneath the normal UnicodeSet
*   and other classes, such as "compiled", fast, read-only (immutable)
*   versions of UnicodeSet.
*/

class UnicodeContainable {
public:
    virtual ~UnicodeContainable() {}

    virtual UBool contains(UChar32 c) const = 0;

    virtual int32_t span(const UChar *s, int32_t length);

    virtual int32_t spanNot(const UChar *s, int32_t length);

    virtual int32_t spanUTF8(const UChar *s, int32_t length);

    virtual int32_t spanNotUTF8(const UChar *s, int32_t length);

    virtual UClassID getDynamicClassID(void) const;
};
