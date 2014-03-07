
/***********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2005, International Business Machines Corporation
 * and others. All Rights Reserved.
 ***********************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "intltest.h"
#include "tfsmalls.h"

#include "unicode/msgfmt.h"
#include "unicode/choicfmt.h"

#include "unicode/parsepos.h"
#include "unicode/fieldpos.h"
#include "unicode/fmtable.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

/*static UBool chkstatus( UErrorCode &status, char* msg = NULL )
{
    UBool ok = (status == U_ZERO_ERROR);
    if (!ok) it_errln( msg );
    return ok;
}*/

void test_ParsePosition( void )
{
    ParsePosition* pp1 = new ParsePosition();
    if (pp1 && (pp1->getIndex() == 0)) {
        it_logln("PP constructor() tested.");
    }else{
        it_errln("*** PP getIndex or constructor() result");
    }
    delete pp1;


    {
        int32_t to = 5;
        ParsePosition pp2( to );
        if (pp2.getIndex() == 5) {
            it_logln("PP getIndex and constructor(int32_t) tested.");
        }else{
            it_errln("*** PP getIndex or constructor(int32_t) result");
        }
        pp2.setIndex( 3 );
        if (pp2.getIndex() == 3) {
            it_logln("PP setIndex tested.");
        }else{
            it_errln("*** PP getIndex or setIndex result");
        }
    }

    ParsePosition pp2, pp3;
    pp2 = 3;
    pp3 = 5;
    ParsePosition pp4( pp3 );
    if ((pp2 != pp3) && (pp3 == pp4)) {
        it_logln("PP copy contructor, operator== and operator != tested.");
    }else{
        it_errln("*** PP operator== or operator != result");
    }

    ParsePosition pp5;
    pp5 = pp4;
    if ((pp4 == pp5) && (!(pp4 != pp5))) {
        it_logln("PP operator= tested.");
    }else{
        it_errln("*** PP operator= operator== or operator != result");
    }


}

#include "unicode/decimfmt.h"

void test_FieldPosition_example( void )
{
    //***** no error detection yet !!!!!!!
    //***** this test is for compiler checks and visual verification only.
    double doubleNum[] = { 123456789.0, -12345678.9, 1234567.89, -123456.789,
        12345.6789, -1234.56789, 123.456789, -12.3456789, 1.23456789};
    int32_t dNumSize = (int32_t)(sizeof(doubleNum)/sizeof(double));

    UErrorCode status = U_ZERO_ERROR;
    DecimalFormat* fmt = (DecimalFormat*) NumberFormat::createInstance(status);
    if (U_FAILURE(status)) {
        it_dataerrln("NumberFormat::createInstance() error");
        return;
    }
    fmt->setDecimalSeparatorAlwaysShown(TRUE);
    
    const int32_t tempLen = 20;
    char temp[tempLen];
    
    for (int32_t i=0; i<dNumSize; i++) {
        FieldPosition pos(NumberFormat::INTEGER_FIELD);
        UnicodeString buf;
        //char fmtText[tempLen];
        //ToCharString(fmt->format(doubleNum[i], buf, pos), fmtText);
        UnicodeString res = fmt->format(doubleNum[i], buf, pos);
        for (int32_t j=0; j<tempLen; j++) temp[j] = '='; // clear with spaces
        int32_t tempOffset = (tempLen <= (tempLen - pos.getEndIndex())) ? 
            tempLen : (tempLen - pos.getEndIndex());
        temp[tempOffset] = '\0';
        it_logln(UnicodeString("FP ") + UnicodeString(temp) + res);
    }
    delete fmt;
    
    it_logln("");

}

void test_FieldPosition( void )
{

    FieldPosition fp( 7 );

    if (fp.getField() == 7) {
        it_logln("FP constructor(int32_t) and getField tested.");
    }else{
        it_errln("*** FP constructor(int32_t) or getField");
    }

    FieldPosition* fph = new FieldPosition( 3 );
    if ( fph->getField() != 3) it_errln("*** FP getField or heap constr.");
    delete fph;

    UBool err1 = FALSE;
    UBool err2 = FALSE;
    UBool err3 = FALSE;
    for (int32_t i = -50; i < 50; i++ ) {
        fp.setField( i+8 );
        fp.setBeginIndex( i+6 );
        fp.setEndIndex( i+7 );
        if (fp.getField() != i+8)  err1 = TRUE;
        if (fp.getBeginIndex() != i+6) err2 = TRUE;
        if (fp.getEndIndex() != i+7) err3 = TRUE;
    }
    if (!err1) {
        it_logln("FP setField and getField tested.");
    }else{
        it_errln("*** FP setField or getField");
    }
    if (!err2) {
        it_logln("FP setBeginIndex and getBeginIndex tested.");
    }else{
        it_errln("*** FP setBeginIndex or getBeginIndex");
    }
    if (!err3) {
        it_logln("FP setEndIndex and getEndIndex tested.");
    }else{
        it_errln("*** FP setEndIndex or getEndIndex");
    }

    it_logln("");

}

void test_Formattable( void )
{
    UErrorCode status = U_ZERO_ERROR;
    Formattable* ftp = new Formattable();
    if (!ftp || !(ftp->getType() == Formattable::kLong) || !(ftp->getLong() == 0)) {
        it_errln("*** Formattable constructor or getType or getLong");
    }
    delete ftp;

    Formattable fta, ftb;
    fta.setLong(1); ftb.setLong(2);
    if ((fta != ftb) || !(fta == ftb)) {
        it_logln("FT setLong, operator== and operator!= tested.");
        status = U_ZERO_ERROR;
        fta.getLong(&status);
        if ( status == U_INVALID_FORMAT_ERROR){
            it_errln("*** FT getLong(UErrorCode* status) failed on real Long");
        } else {
            it_logln("FT getLong(UErrorCode* status) tested.");
        }
    }else{
        it_errln("*** Formattable setLong or operator== or !=");
    }
    fta = ftb;
    if ((fta == ftb) || !(fta != ftb)) {
        it_logln("FT operator= tested.");
    }else{
        it_errln("*** FT operator= or operator== or operator!=");
    }
    
    fta.setDouble( 3.0 );
    if ((fta.getType() == Formattable::kDouble) && (fta.getDouble() == 3.0)) {
        it_logln("FT set- and getDouble tested.");
    }else{
        it_errln("*** FT set- or getDouble");
    }
    
    fta.getDate(status = U_ZERO_ERROR);
    if (status != U_INVALID_FORMAT_ERROR){
        it_errln("*** FT getDate with status should fail on non-Date");
    }
    fta.setDate( 4.0 );
    if ((fta.getType() == Formattable::kDate) && (fta.getDate() == 4.0)) {
        it_logln("FT set- and getDate tested.");	  
        status = U_ZERO_ERROR;
        fta.getDate(status);
        if ( status == U_INVALID_FORMAT_ERROR){
            it_errln("*** FT getDate with status failed on real Date");
        } else {
            it_logln("FT getDate with status tested.");
        }
    }else{
        it_errln("*** FT set- or getDate");
    }

    status = U_ZERO_ERROR;
    fta.getLong(&status);
    if (status != U_INVALID_FORMAT_ERROR){
        it_errln("*** FT getLong(UErrorCode* status) should fail on non-Long");
    }

    fta.setString("abc");
    const Formattable ftc(fta);
    UnicodeString res;

    {
        UBool t;
        t = (fta.getType() == Formattable::kString) 
            && (fta.getString(res) == "abc")
            && (fta.getString() == "abc");
        res = fta.getString(status = U_ZERO_ERROR);
        t = t && (status != U_INVALID_FORMAT_ERROR && res == "abc");
        res = ftc.getString(status = U_ZERO_ERROR);
        t = t && (status != U_INVALID_FORMAT_ERROR && res == "abc");
        ftc.getString(res,status = U_ZERO_ERROR);
        t = t && (status != U_INVALID_FORMAT_ERROR && res == "abc"); 
        if (t) {
            it_logln("FT set- and getString tested.");
        }else{
            it_errln("*** FT set- or getString");
        }
    }

    UnicodeString ucs = "unicode-string";
    UnicodeString* ucs_ptr = new UnicodeString("pointed-to-unicode-string");

    const Formattable ftarray[] = 
    {
        Formattable( 1.0, Formattable::kIsDate ),
        2.0,
        (int32_t)3,
        ucs,
        ucs_ptr
    };
    const int32_t ft_cnt = LENGTHOF(ftarray);
    Formattable ft_arr( ftarray, ft_cnt );
    UnicodeString temp;
    if ((ft_arr[0].getType() == Formattable::kDate)   && (ft_arr[0].getDate()   == 1.0)
     && (ft_arr[1].getType() == Formattable::kDouble) && (ft_arr[1].getDouble() == 2.0)
     && (ft_arr[2].getType() == Formattable::kLong)   && (ft_arr[2].getLong()   == (int32_t)3)
     && (ft_arr[3].getType() == Formattable::kString) && (ft_arr[3].getString(temp) == ucs)
     && (ft_arr[4].getType() == Formattable::kString) && (ft_arr[4].getString(temp) == *ucs_ptr) ) {
        it_logln("FT constr. for date, double, long, ustring, ustring* and array tested");
    }else{
        it_errln("*** FT constr. for date, double, long, ustring, ustring* or array");
    }

    int32_t i, res_cnt;
    const Formattable* res_array = ft_arr.getArray( res_cnt );
    if (res_cnt == ft_cnt) {
        UBool same  = TRUE;
        for (i = 0; i < res_cnt; i++ ) {
            if (res_array[i] != ftarray[i]) {
                same = FALSE;
            }
        }
        if (same) {
            it_logln("FT getArray tested");
            res_array = ft_arr.getArray( res_cnt, status = U_ZERO_ERROR);
            if (status == U_INVALID_FORMAT_ERROR){
                it_errln("*** FT getArray with status failed on real array");
            } else {
                it_logln("FT getArray with status tested on real array");
            }
        }else{
            it_errln("*** FT getArray comparison");
        }
    }else{
        it_errln(UnicodeString("*** FT getArray count res_cnt=") + res_cnt + UnicodeString("ft_cnt=") + ft_cnt);
    }
    
    res_array = fta.getArray(res_cnt, status = U_ZERO_ERROR);
    if (status == U_INVALID_FORMAT_ERROR){
        if (res_cnt == 0 && res_array == NULL){
            it_logln("FT getArray with status tested on non array");
        } else {
            it_errln("*** FT getArray with status return values are not consistent");
        }
    } else {
        it_errln("*** FT getArray with status should fail on non-array");
    }


    Formattable *pf;
    for(i = 0; i < ft_cnt; ++i) {
        pf = ftarray[i].clone();
        if(pf == (ftarray + i) || *pf != ftarray[i]) {
            it_errln("Formattable.clone() failed for item %d" + i);
        }
        delete pf;
    }

    const Formattable ftarr1[] = { Formattable( (int32_t)1 ), Formattable( (int32_t)2 ) };
    const Formattable ftarr2[] = { Formattable( (int32_t)3 ), Formattable( (int32_t)4 ) };

    const int32_t ftarr1_cnt = (int32_t)(sizeof(ftarr1) / sizeof(Formattable));
    const int32_t ftarr2_cnt = (int32_t)(sizeof(ftarr2) / sizeof(Formattable));

    ft_arr.setArray( ftarr1, ftarr1_cnt );
    if ((ft_arr[0].getType() == Formattable::kLong) && (ft_arr[0].getLong() == (int32_t)1)) {
        it_logln("FT setArray tested");
    }else{
        it_errln("*** FT setArray");
    }

    Formattable* ft_dynarr = new Formattable[ftarr2_cnt];
    for (i = 0; i < ftarr2_cnt; i++ ) {
        ft_dynarr[i] = ftarr2[i];
    }
    if ((ft_dynarr[0].getType() == Formattable::kLong) && (ft_dynarr[0].getLong() == (int32_t)3)
     && (ft_dynarr[1].getType() == Formattable::kLong) && (ft_dynarr[1].getLong() == (int32_t)4)) {
        it_logln("FT operator= and array operations tested");
    }else{
        it_errln("*** FT operator= or array operations");
    }

    ft_arr.adoptArray( ft_dynarr, ftarr2_cnt );
    if ((ft_arr[0].getType() == Formattable::kLong) && (ft_arr[0].getLong() == (int32_t)3)
     && (ft_arr[1].getType() == Formattable::kLong) && (ft_arr[1].getLong() == (int32_t)4)) {
        it_logln("FT adoptArray tested");
    }else{
        it_errln("*** FT adoptArray or operator[]");
    }

    ft_arr.setLong(0);   // calls 'dispose' and deletes adopted array !

    UnicodeString* ucs_dyn = new UnicodeString("ttt");
    UnicodeString tmp2;

    fta.adoptString( ucs_dyn );
    if ((fta.getType() == Formattable::kString) && (fta.getString(tmp2) == "ttt")) {
        it_logln("FT adoptString tested");
    }else{
        it_errln("*** FT adoptString or getString");
    }
    fta.setLong(0);   // calls 'dispose' and deletes adopted string !

    it_logln();
}

void TestFormatSmallClasses::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    switch (index) {
        case 0: name = "pp"; 
                if (exec) logln("TestSuite Format/SmallClasses/ParsePosition (f/chc/sma/pp): ");
                if (exec) test_ParsePosition(); 
                break;
        case 1: name = "fp"; 
                if (exec) logln("TestSuite Format/SmallClasses/FieldPosition (f/chc/sma/fp): ");
                if (exec) test_FieldPosition(); 
                break;
        case 2: name = "fpe"; 
                if (exec) logln("TestSuite Format/SmallClasses/FieldPositionExample (f/chc/sma/fpe): ");
                if (exec) test_FieldPosition_example(); 
                break;
        case 3: name = "ft"; 
                if (exec) logln("TestSuite Format/SmallClasses/Formattable (f/chc/sma/ft): ");
                if (exec) test_Formattable(); 
                break;
        default: name = ""; break; //needed to end loop
    }
}

#endif /* #if !UCONFIG_NO_FORMATTING */
