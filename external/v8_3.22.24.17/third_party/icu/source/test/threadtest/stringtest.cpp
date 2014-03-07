//
//********************************************************************
//   Copyright (C) 2002, International Business Machines
//   Corporation and others.  All Rights Reserved.
//********************************************************************
//
// File stringtest.cpp
//

#include "threadtest.h"
#include "unicode/unistr.h"
#include "stdio.h"

class StringThreadTest: public AbstractThreadTest {
public:
                    StringThreadTest();
    virtual        ~StringThreadTest();
    virtual void    check();
    virtual void    runOnce();
            void    makeStringCopies(int recursionCount);

private:
    UnicodeString   *fCleanStrings;
    UnicodeString   *fSourceStrings;
};

StringThreadTest::StringThreadTest() {
    // cleanStrings and sourceStrings are separately initialized to the same values.
    // cleanStrings are never touched after in any remotely unsafe way.
    // sourceStrings are copied during the test, which will run their buffer's reference
    //    counts all over the place.
    fCleanStrings     = new UnicodeString[5];
    fSourceStrings    = new UnicodeString[5];

    fCleanStrings[0]  = "When sorrows come, they come not single spies, but in batallions.";
    fSourceStrings[0] = "When sorrows come, they come not single spies, but in batallions.";
    fCleanStrings[1]  = "Away, you scullion! You rampallion! You fustilarion! I'll tickle your catastrophe!";
    fSourceStrings[1] = "Away, you scullion! You rampallion! You fustilarion! I'll tickle your catastrophe!"; 
    fCleanStrings[2]  = "hot";
    fSourceStrings[2] = "hot"; 
    fCleanStrings[3]  = "";
    fSourceStrings[3] = ""; 
    fCleanStrings[4]  = "Tomorrow, and tomorrow, and tomorrow,\n"
                        "Creeps in this petty pace from day to day\n"
                        "To the last syllable of recorded time;\n"
                        "And all our yesterdays have lighted fools \n"
                        "The way to dusty death. Out, out brief candle!\n"
                        "Life's but a walking shadow, a poor player\n"
                        "That struts and frets his hour upon the stage\n"
                        "And then is heard no more. It is a tale\n"
                        "Told by and idiot, full of sound and fury,\n"
                        "Signifying nothing.\n";
    fSourceStrings[4] = "Tomorrow, and tomorrow, and tomorrow,\n"
                        "Creeps in this petty pace from day to day\n"
                        "To the last syllable of recorded time;\n"
                        "And all our yesterdays have lighted fools \n"
                        "The way to dusty death. Out, out brief candle!\n"
                        "Life's but a walking shadow, a poor player\n"
                        "That struts and frets his hour upon the stage\n"
                        "And then is heard no more. It is a tale\n"
                        "Told by and idiot, full of sound and fury,\n"
                        "Signifying nothing.\n";
};


StringThreadTest::~StringThreadTest() {
    delete [] fCleanStrings;
    delete [] fSourceStrings;
}


void   StringThreadTest::runOnce() {
    makeStringCopies(25);
}

void   StringThreadTest::makeStringCopies(int recursionCount) {
    UnicodeString firstGeneration[5];
    UnicodeString secondGeneration[5];
    UnicodeString thirdGeneration[5];
    UnicodeString fourthGeneration[5];

    // Make four generations of copies of the source strings, in slightly variant ways.
    //
    int i;
    for (i=0; i<5; i++) {
         firstGeneration[i]   = fSourceStrings[i];
         secondGeneration[i]  = firstGeneration[i];
         thirdGeneration[i]   = UnicodeString(secondGeneration[i]);
 //        fourthGeneration[i]  = UnicodeString("Lay on, MacDuff, And damn'd be him that first cries, \"Hold, enough!\"");
         fourthGeneration[i]  = UnicodeString();
         fourthGeneration[i]  = thirdGeneration[i];
    }


    // Recurse to make even more copies of the strings/
    //
    if (recursionCount > 0) {
        makeStringCopies(recursionCount-1);
    }


    // Verify that all four generations are equal.
    for (i=0; i<5; i++) {
        if (firstGeneration[i] !=  fSourceStrings[i]   ||
            firstGeneration[i] !=  secondGeneration[i] ||
            firstGeneration[i] !=  thirdGeneration[i]  ||
            firstGeneration[i] !=  fourthGeneration[i])
        {
            fprintf(stderr, "Error, strings don't compare equal.\n");
        }
    }

};
  

void   StringThreadTest::check() {
    //
    //  Check that the reference counts on the buffers for all of the source strings
    //     are one.   The ref counts will have run way up while the test threads
    //     make numerous copies of the strings, but at the top of the loop all
    //     of the copies should be gone.
    //
    int i;

    for (i=0; i<5; i++) {
        if (fSourceStrings[i].fFlags & UnicodeString::kRefCounted) {
            const UChar *buf = fSourceStrings[i].getBuffer();
            uint32_t refCount = fSourceStrings[i].refCount();
            if (refCount != 1) {
                fprintf(stderr, "\nFailure.  SourceString Ref Count was %d, should be 1.\n", refCount);
            }
        }
    }
};
  

//
//  Factory functoin for StringThreadTest.
//     C function lets ThreadTest create StringTests without needing StringThreadTest header.
// 
AbstractThreadTest  *createStringTest() {
    return new StringThreadTest();
};
