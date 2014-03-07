/********************************************************************
* COPYRIGHT:
* Copyright (C) 2001-2006 IBM, Inc.   All Rights Reserved.
*
********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <limits.h>
#include <string.h>
#include "unicode/uperf.h"
#include "uoptions.h"
#include "unicode/coll.h"
#include <unicode/ucoleitr.h>



/* To store an array of string<UNIT> in continue space.
Since string<UNIT> itself is treated as an array of UNIT, this 
class will ease our memory management for an array of string<UNIT>.
*/

//template<typename UNIT> 
#define COMPATCT_ARRAY(CompactArrays, UNIT) \
struct CompactArrays{\
    CompactArrays(const CompactArrays & );\
    CompactArrays & operator=(const CompactArrays & );\
    int32_t   count;/*total number of the strings*/ \
    int32_t * index;/*relative offset in data*/ \
    UNIT    * data; /*the real space to hold strings*/ \
    \
    ~CompactArrays(){free(index);free(data);} \
    CompactArrays():data(NULL), index(NULL), count(0){ \
    index = (int32_t *) realloc(index, sizeof(int32_t)); \
    index[0] = 0; \
    } \
    void append_one(int32_t theLen){ /*include terminal NULL*/ \
    count++; \
    index = (int32_t *) realloc(index, sizeof(int32_t) * (count + 1)); \
    index[count] = index[count - 1] + theLen; \
    data = (UNIT *) realloc(data, sizeof(UNIT) * index[count]); \
    } \
    UNIT * last(){return data + index[count - 1];} \
    UNIT * dataOf(int32_t i){return data + index[i];} \
    int32_t lengthOf(int i){return index[i+1] - index[i] - 1; }	/*exclude terminating NULL*/  \
};

//typedef CompactArrays<UChar> CA_uchar;
//typedef CompactArrays<char> CA_char;
//typedef CompactArrays<uint8_t> CA_uint8;
//typedef CompactArrays<WCHAR> CA_win_wchar;

COMPATCT_ARRAY(CA_uchar, UChar)
COMPATCT_ARRAY(CA_char, char)
COMPATCT_ARRAY(CA_uint8, uint8_t)
COMPATCT_ARRAY(CA_win_wchar, WCHAR)


struct DataIndex {
    static DWORD        win_langid;     // for qsort callback function
    static UCollator *  col;            // for qsort callback function
    uint8_t *   icu_key;
    UChar *     icu_data;
    int32_t     icu_data_len;
    char*       posix_key;
    char*       posix_data;
    int32_t     posix_data_len;
    char*       win_key;
    WCHAR *     win_data;
    int32_t     win_data_len;
};
DWORD DataIndex::win_langid;
UCollator * DataIndex::col;



class CmdKeyGen : public UPerfFunction {
    typedef	void (CmdKeyGen::* Func)(int32_t);
    enum{MAX_KEY_LENGTH = 5000};
    UCollator * col;
    DWORD       win_langid;
    int32_t     count;
    DataIndex * data;
    Func 	    fn; 

    union { // to save sapce
        uint8_t		icu_key[MAX_KEY_LENGTH];
        char        posix_key[MAX_KEY_LENGTH];
        WCHAR		win_key[MAX_KEY_LENGTH];
    };
public:
    CmdKeyGen(UErrorCode, UCollator * col,DWORD win_langid, int32_t count, DataIndex * data,Func fn,int32_t)
        :col(col),win_langid(win_langid), count(count), data(data), fn(fn){}

        virtual long getOperationsPerIteration(){return count;}

        virtual void call(UErrorCode* status){
            for(int32_t i = 0; i< count; i++){
                (this->*fn)(i);
            }
        }

        void icu_key_null(int32_t i){
            ucol_getSortKey(col, data[i].icu_data, -1, icu_key, MAX_KEY_LENGTH);
        }

        void icu_key_len(int32_t i){
            ucol_getSortKey(col, data[i].icu_data, data[i].icu_data_len, icu_key, MAX_KEY_LENGTH);
        }

        // pre-generated in CollPerfTest::prepareData(), need not to check error here
        void win_key_null(int32_t i){
            //LCMAP_SORTsk             0x00000400  // WC sort sk (normalize)
            LCMapStringW(win_langid, LCMAP_SORTKEY, data[i].win_data, -1, win_key, MAX_KEY_LENGTH);
        }

        void win_key_len(int32_t i){
            LCMapStringW(win_langid, LCMAP_SORTKEY, data[i].win_data, data[i].win_data_len, win_key, MAX_KEY_LENGTH);
        }

        void posix_key_null(int32_t i){
            strxfrm(posix_key, data[i].posix_data, MAX_KEY_LENGTH);
        }
};


class CmdIter : public UPerfFunction {
    typedef	void (CmdIter::* Func)(UErrorCode* , int32_t );
    int32_t             count;
    CA_uchar *          data;
    Func                fn;
    UCollationElements *iter;
    int32_t             exec_count;
public:
    CmdIter(UErrorCode & status, UCollator * col, int32_t count, CA_uchar *data, Func fn, int32_t,int32_t)
        :count(count), data(data), fn(fn){
            exec_count = 0;
            UChar dummytext[] = {0, 0};
            iter = ucol_openElements(col, NULL, 0, &status);
            ucol_setText(iter, dummytext, 1, &status);
        }
        ~CmdIter(){
            ucol_closeElements(iter);
        }

        virtual long getOperationsPerIteration(){return exec_count ? exec_count : 1;}

        virtual void call(UErrorCode* status){
            exec_count = 0;
            for(int32_t i = 0; i< count; i++){
                (this->*fn)(status, i);
            }
        }

        void icu_forward_null(UErrorCode* status, int32_t i){
            ucol_setText(iter, data->dataOf(i), -1, status);
            while (ucol_next(iter, status) != UCOL_NULLORDER) exec_count++;
        }

        void icu_forward_len(UErrorCode* status, int32_t i){
            ucol_setText(iter, data->dataOf(i), data->lengthOf(i) , status);
            while (ucol_next(iter, status) != UCOL_NULLORDER) exec_count++;
        }

        void icu_backward_null(UErrorCode* status, int32_t i){
            ucol_setText(iter, data->dataOf(i), -1, status);
            while (ucol_previous(iter, status) != UCOL_NULLORDER) exec_count++;
        }

        void icu_backward_len(UErrorCode* status, int32_t i){
            ucol_setText(iter, data->dataOf(i), data->lengthOf(i) , status);
            while (ucol_previous(iter, status) != UCOL_NULLORDER) exec_count++;
        }
};

class CmdIterAll : public UPerfFunction {
    typedef	void (CmdIterAll::* Func)(UErrorCode* status);
    int32_t     count;
    UChar *     data;
    Func        fn;
    UCollationElements *iter;
    int32_t     exec_count;

public:
    enum CALL {forward_null, forward_len, backward_null, backward_len};

    ~CmdIterAll(){
        ucol_closeElements(iter);
    }
    CmdIterAll(UErrorCode & status, UCollator * col, int32_t count,  UChar * data, CALL call,int32_t,int32_t)
        :count(count),data(data)
    {
        exec_count = 0;
        if (call == forward_null || call == backward_null) {
            iter = ucol_openElements(col, data, -1, &status);
        } else {
            iter = ucol_openElements(col, data, count, &status);
        }

        if (call == forward_null || call == forward_len){
            fn = &CmdIterAll::icu_forward_all;
        } else {
            fn = &CmdIterAll::icu_backward_all;
        }
    }
    virtual long getOperationsPerIteration(){return exec_count ? exec_count : 1;}

    virtual void call(UErrorCode* status){
        (this->*fn)(status);
    }

    void icu_forward_all(UErrorCode* status){
        int strlen = count - 5;
        int count5 = 5;
        int strindex = 0;
        ucol_setOffset(iter, strindex, status);
        while (TRUE) {
            if (ucol_next(iter, status) == UCOL_NULLORDER) {
                break;
            }
            exec_count++;
            count5 --;
            if (count5 == 0) {
                strindex += 10;
                if (strindex > strlen) {
                    break;
                }
                ucol_setOffset(iter, strindex, status);
                count5 = 5;
            }
        }
    }

    void icu_backward_all(UErrorCode* status){
        int strlen = count;
        int count5 = 5;
        int strindex = 5;
        ucol_setOffset(iter, strindex, status);
        while (TRUE) {
            if (ucol_previous(iter, status) == UCOL_NULLORDER) {
                break;
            }
            exec_count++;
            count5 --;
            if (count5 == 0) {
                strindex += 10;
                if (strindex > strlen) {
                    break;
                }
                ucol_setOffset(iter, strindex, status);
                count5 = 5;
            }
        }
    }

};

struct CmdQsort : public UPerfFunction{

    static int q_random(const void * a, const void * b){
        uint8_t * key_a = ((DataIndex *)a)->icu_key;
        uint8_t * key_b = ((DataIndex *)b)->icu_key;

        int   val_a = 0;
        int   val_b = 0;
        while (*key_a != 0) {val_a += val_a*37 + *key_a++;}
        while (*key_b != 0) {val_b += val_b*37 + *key_b++;}
        return val_a - val_b;
    }

#define QCAST() \
    DataIndex * da = (DataIndex *) a; \
    DataIndex * db = (DataIndex *) b; \
    ++exec_count

    static int icu_strcoll_null(const void *a, const void *b){
        QCAST(); 
        return ucol_strcoll(da->col, da->icu_data, -1, db->icu_data, -1) - UCOL_EQUAL;
    }

    static int icu_strcoll_len(const void *a, const void *b){
        QCAST();
        return ucol_strcoll(da->col, da->icu_data, da->icu_data_len, db->icu_data, db->icu_data_len) - UCOL_EQUAL;
    }

    static int icu_cmpkey (const void *a, const void *b){ 
        QCAST(); 
        return strcmp((char *) da->icu_key, (char *) db->icu_key); 
    }

    static int win_cmp_null(const void *a, const void *b) {
        QCAST();
        //CSTR_LESS_THAN		1
        //CSTR_EQUAL			2
        //CSTR_GREATER_THAN		3
        int t = CompareStringW(da->win_langid, 0, da->win_data, -1, db->win_data, -1);
        if (t == 0){
            fprintf(stderr, "CompareStringW error, error number %x\n", GetLastError());
            exit(-1);
        } else{
            return t - CSTR_EQUAL;
        }
    }

    static int win_cmp_len(const void *a, const void *b) {
        QCAST();
        int t = CompareStringW(da->win_langid, 0, da->win_data, da->win_data_len, db->win_data, db->win_data_len);
        if (t == 0){
            fprintf(stderr, "CompareStringW error, error number %x\n", GetLastError());
            exit(-1);
        } else{
            return t - CSTR_EQUAL;
        }
    }

#define QFUNC(name, func, data) \
    static int name (const void *a, const void *b){ \
    QCAST(); \
    return func(da->data, db->data); \
    }

    QFUNC(posix_strcoll_null, strcoll, posix_data)
        QFUNC(posix_cmpkey, strcmp, posix_key)
        QFUNC(win_cmpkey, strcmp, win_key)
        QFUNC(win_wcscmp, wcscmp, win_data)
        QFUNC(icu_strcmp, u_strcmp, icu_data)
        QFUNC(icu_cmpcpo, u_strcmpCodePointOrder, icu_data)

private:        
    static int32_t exec_count; // potential muilt-thread problem

    typedef	int (* Func)(const void *, const void *);

    Func    fn;
    void *  base;   //Start of target array. 
    int32_t num;    //Array size in elements. 
    int32_t width;  //Element size in bytes. 

    void *  backup; //copy source of base
public:
    CmdQsort(UErrorCode & status,void *theBase, int32_t num, int32_t width, Func fn, int32_t,int32_t)
        :backup(theBase),num(num),width(width),fn(fn){
            base = malloc(num * width);
            time_empty(100, &status); // warm memory/cache
        }

        ~CmdQsort(){
            free(base);
        }

        void empty_call(){
            exec_count = 0;
            memcpy(base, backup, num * width);
        }

        double time_empty(int32_t n, UErrorCode* status) {
            UTimer start, stop;
            utimer_getTime(&start); 
            while (n-- > 0) {
                empty_call();
            }
            utimer_getTime(&stop);
            return utimer_getDeltaSeconds(&start,&stop); // ms
        }

        virtual void call(UErrorCode* status){
            exec_count = 0;
            memcpy(base, backup, num * width);
            qsort(base, num, width, fn);
        }
        virtual double time(int32_t n, UErrorCode* status) {
            double t1 = time_empty(n,status);
            double t2 = UPerfFunction::time(n, status);
            return  t2-t1;// < 0 ? t2 : t2-t1;
        }

        virtual long getOperationsPerIteration(){ return exec_count?exec_count:1;}
};
int32_t CmdQsort::exec_count;


class CmdBinSearch : public UPerfFunction{
public:
    typedef	int (CmdBinSearch::* Func)(int, int);

    UCollator * col;
    DWORD       win_langid;
    int32_t     count;
    DataIndex * rnd;
    DataIndex * ord;
    Func 	    fn;
    int32_t     exec_count;

    CmdBinSearch(UErrorCode, UCollator * col,DWORD win_langid,int32_t count,DataIndex * rnd,DataIndex * ord,Func fn)
        :col(col),win_langid(win_langid), count(count), rnd(rnd), ord(ord), fn(fn),exec_count(0){}


        virtual void call(UErrorCode* status){
            exec_count = 0;
            for(int32_t i = 0; i< count; i++){ // search all data
                binary_search(i);
            }
        }
        virtual long getOperationsPerIteration(){ return exec_count?exec_count:1;}

        void binary_search(int32_t random)	{
            int low   = 0;
            int high  = count - 1;
            int guess;
            int last_guess = -1;
            int r;
            while (TRUE) {
                guess = (high + low)/2;
                if (last_guess == guess) break; // nothing to search

                r = (this->*fn)(random, guess);
                exec_count++;

                if (r == 0)	
                    return;	// found, search end.
                if (r < 0) {
                    high = guess;
                } else {
                    low  = guess;
                }
                last_guess = guess;
            } 
        }

        int icu_strcoll_null(int32_t i, int32_t j){
            return ucol_strcoll(col, rnd[i].icu_data, -1, ord[j].icu_data,-1);
        }

        int icu_strcoll_len(int32_t i, int32_t j){
            return ucol_strcoll(col, rnd[i].icu_data, rnd[i].icu_data_len, ord[j].icu_data, ord[j].icu_data_len);
        }

        int icu_cmpkey(int32_t i, int32_t j) {
            return strcmp( (char *) rnd[i].icu_key, (char *) ord[j].icu_key );
        }

        int win_cmp_null(int32_t i, int32_t j) {
            int t = CompareStringW(win_langid, 0, rnd[i].win_data, -1, ord[j].win_data, -1);
            if (t == 0){
                fprintf(stderr, "CompareStringW error, error number %x\n", GetLastError());
                exit(-1);
            } else{
                return t - CSTR_EQUAL;
            }
        }

        int win_cmp_len(int32_t i, int32_t j) {
            int t = CompareStringW(win_langid, 0, rnd[i].win_data, rnd[i].win_data_len, ord[j].win_data, ord[j].win_data_len);
            if (t == 0){
                fprintf(stderr, "CompareStringW error, error number %x\n", GetLastError());
                exit(-1);
            } else{
                return t - CSTR_EQUAL;
            }
        }

#define BFUNC(name, func, data) \
    int name(int32_t i, int32_t j) { \
    return func(rnd[i].data, ord[j].data); \
    }

        BFUNC(posix_strcoll_null, strcoll, posix_data)
            BFUNC(posix_cmpkey, strcmp, posix_key)
            BFUNC(win_cmpkey, strcmp, win_key)
            BFUNC(win_wcscmp, wcscmp, win_data)
            BFUNC(icu_strcmp, u_strcmp, icu_data)
            BFUNC(icu_cmpcpo, u_strcmpCodePointOrder, icu_data)
};

class CollPerfTest : public UPerfTest {
public:
    UCollator *     col;
    DWORD           win_langid;

    UChar * icu_data_all;
    int32_t icu_data_all_len;

    int32_t         count;
    CA_uchar *      icu_data;
    CA_uint8 *      icu_key;
    CA_char *       posix_data;
    CA_char *       posix_key;
    CA_win_wchar *  win_data;
    CA_char *       win_key;

    DataIndex * rnd_index; // random by icu key
    DataIndex * ord_win_data;
    DataIndex * ord_win_key;
    DataIndex * ord_posix_data;
    DataIndex * ord_posix_key;
    DataIndex * ord_icu_data;
    DataIndex * ord_icu_key;
    DataIndex * ord_win_wcscmp;
    DataIndex * ord_icu_strcmp;
    DataIndex * ord_icu_cmpcpo;

    virtual ~CollPerfTest(){
        ucol_close(col);
        delete [] icu_data_all;
        delete icu_data;
        delete icu_key;
        delete posix_data;
        delete posix_key;
        delete win_data;
        delete win_key;
        delete[] rnd_index;
        delete[] ord_win_data;
        delete[] ord_win_key;
        delete[] ord_posix_data;
        delete[] ord_posix_key;
        delete[] ord_icu_data;
        delete[] ord_icu_key;
        delete[] ord_win_wcscmp;
        delete[] ord_icu_strcmp;
        delete[] ord_icu_cmpcpo;
    }

    CollPerfTest(int32_t argc, const char* argv[], UErrorCode& status):UPerfTest(argc, argv, status){
        col = NULL;
        icu_data_all = NULL;
        icu_data = NULL;
        icu_key = NULL;
        posix_data = NULL;
        posix_key = NULL;
        win_data =NULL;
        win_key = NULL;

        rnd_index = NULL;
        ord_win_data= NULL;
        ord_win_key= NULL;
        ord_posix_data= NULL;
        ord_posix_key= NULL;
        ord_icu_data= NULL;
        ord_icu_key= NULL;
        ord_win_wcscmp = NULL;
        ord_icu_strcmp = NULL;
        ord_icu_cmpcpo = NULL;

        if (U_FAILURE(status)){
            return;
        }

        // Parse additional arguments

        UOption options[] = {
            UOPTION_DEF("langid", 'i', UOPT_REQUIRES_ARG),        // Windows Language ID number.
                UOPTION_DEF("rulefile", 'r', UOPT_REQUIRES_ARG),      // --rulefile <filename>
                // Collation related arguments. All are optional.
                // To simplify parsing, two choice arguments are disigned as NO_ARG.
                // The default value is UPPER word in the comment
                UOPTION_DEF("c_french", 'f', UOPT_NO_ARG),          // --french <on | OFF>
                UOPTION_DEF("c_alternate", 'a', UOPT_NO_ARG),       // --alternate <NON_IGNORE | shifted>
                UOPTION_DEF("c_casefirst", 'c', UOPT_REQUIRES_ARG), // --casefirst <lower | upper | OFF>
                UOPTION_DEF("c_caselevel", 'l', UOPT_NO_ARG),       // --caselevel <on | OFF>
                UOPTION_DEF("c_normal", 'n', UOPT_NO_ARG),          // --normal <on | OFF> 
                UOPTION_DEF("c_strength", 's', UOPT_REQUIRES_ARG),  // --strength <1-5>
        };
        int32_t opt_len = (sizeof(options)/sizeof(options[0]));
        enum {i, r,f,a,c,l,n,s};   // The buffer between the option items' order and their references

        _remainingArgc = u_parseArgs(_remainingArgc, (char**)argv, opt_len, options);

        if (_remainingArgc < 0){
            status = U_ILLEGAL_ARGUMENT_ERROR;
            return;
        }

        if (locale == NULL){
            locale = "en_US";   // set default locale
        }

        //#ifdef U_WINDOWS
        if (options[i].doesOccur) {
            char *endp;
            int tmp = strtol(options[i].value, &endp, 0);
            if (endp == options[i].value) {
                status = U_ILLEGAL_ARGUMENT_ERROR;
                return;
            }
            win_langid = MAKELCID(tmp, SORT_DEFAULT);
        } else {
            win_langid = uloc_getLCID(locale);
        }
        //#endif

        //  Set up an ICU collator
        if (options[r].doesOccur) {
            // TODO: implement it
        } else {
            col = ucol_open(locale, &status);
            if (U_FAILURE(status)) {
                return;
            }
        }

        if (options[f].doesOccur) {
            ucol_setAttribute(col, UCOL_FRENCH_COLLATION, UCOL_ON, &status);
        } else {
            ucol_setAttribute(col, UCOL_FRENCH_COLLATION, UCOL_OFF, &status);
        }

        if (options[a].doesOccur) {
            ucol_setAttribute(col, UCOL_ALTERNATE_HANDLING, UCOL_SHIFTED, &status);
        }

        if (options[c].doesOccur) { // strcmp() has i18n encoding problem
            if (strcmp("lower", options[c].value) == 0){
                ucol_setAttribute(col, UCOL_CASE_FIRST, UCOL_LOWER_FIRST, &status);
            } else if (strcmp("upper", options[c].value) == 0) {
                ucol_setAttribute(col, UCOL_CASE_FIRST, UCOL_UPPER_FIRST, &status);
            } else {
                status = U_ILLEGAL_ARGUMENT_ERROR;
                return;
            }
        }

        if (options[l].doesOccur){
            ucol_setAttribute(col, UCOL_CASE_LEVEL, UCOL_ON, &status);
        }

        if (options[n].doesOccur){
            ucol_setAttribute(col, UCOL_NORMALIZATION_MODE, UCOL_ON, &status);
        }

        if (options[s].doesOccur) {
            char *endp;
            int tmp = strtol(options[l].value, &endp, 0);
            if (endp == options[l].value) {
                status = U_ILLEGAL_ARGUMENT_ERROR;
                return;
            }
            switch (tmp) {
            case 1:	ucol_setAttribute(col, UCOL_STRENGTH, UCOL_PRIMARY, &status);		break;
            case 2:	ucol_setAttribute(col, UCOL_STRENGTH, UCOL_SECONDARY, &status);		break;
            case 3:	ucol_setAttribute(col, UCOL_STRENGTH, UCOL_TERTIARY, &status);		break;
            case 4:	ucol_setAttribute(col, UCOL_STRENGTH, UCOL_QUATERNARY, &status);	break;
            case 5:	ucol_setAttribute(col, UCOL_STRENGTH, UCOL_IDENTICAL, &status);		break;
            default: status = U_ILLEGAL_ARGUMENT_ERROR;					return;
            }
        }
        prepareData(status);
    }

    //to avoid use the annoying 'id' in TESTCASE(id,test) macro or the like
#define TEST(testname, classname, arg1, arg2, arg3, arg4, arg5, arg6) \
    if(temp == index) {\
    name = #testname;\
    if (exec) {\
    UErrorCode status = U_ZERO_ERROR;\
    UPerfFunction * t = new classname(status,arg1, arg2, arg3, arg4, arg5, arg6);\
    if (U_FAILURE(status)) {\
    delete t;\
    return NULL;\
    } else {\
    return t;\
    }\
    } else {\
    return NULL;\
    }\
    }\
    temp++\


    virtual UPerfFunction* runIndexedTest( /*[in]*/int32_t index, /*[in]*/UBool exec, /*[out]*/const char* &name, /*[in]*/ char* par = NULL ){
        int temp = 0;

#define TEST_KEYGEN(testname, func)\
    TEST(testname, CmdKeyGen, col, win_langid, count, rnd_index, &CmdKeyGen::func, 0)
        TEST_KEYGEN(TestIcu_KeyGen_null, icu_key_null);
        TEST_KEYGEN(TestIcu_KeyGen_len,  icu_key_len);
        TEST_KEYGEN(TestPosix_KeyGen_null, posix_key_null);
        TEST_KEYGEN(TestWin_KeyGen_null, win_key_null);
        TEST_KEYGEN(TestWin_KeyGen_len, win_key_len);

#define TEST_ITER(testname, func)\
    TEST(testname, CmdIter, col, count, icu_data, &CmdIter::func,0,0)
        TEST_ITER(TestIcu_ForwardIter_null, icu_forward_null);
        TEST_ITER(TestIcu_ForwardIter_len, icu_forward_len);
        TEST_ITER(TestIcu_BackwardIter_null, icu_backward_null);
        TEST_ITER(TestIcu_BackwardIter_len, icu_backward_len);

#define TEST_ITER_ALL(testname, func)\
    TEST(testname, CmdIterAll, col, icu_data_all_len, icu_data_all, CmdIterAll::func,0,0)
        TEST_ITER_ALL(TestIcu_ForwardIter_all_null, forward_null);
        TEST_ITER_ALL(TestIcu_ForwardIter_all_len, forward_len);
        TEST_ITER_ALL(TestIcu_BackwardIter_all_null, backward_null);
        TEST_ITER_ALL(TestIcu_BackwardIter_all_len, backward_len);

#define TEST_QSORT(testname, func)\
    TEST(testname, CmdQsort, rnd_index, count, sizeof(DataIndex), CmdQsort::func,0,0)
        TEST_QSORT(TestIcu_qsort_strcoll_null, icu_strcoll_null);
        TEST_QSORT(TestIcu_qsort_strcoll_len, icu_strcoll_len);
        TEST_QSORT(TestIcu_qsort_usekey, icu_cmpkey);
        TEST_QSORT(TestPosix_qsort_strcoll_null, posix_strcoll_null);
        TEST_QSORT(TestPosix_qsort_usekey, posix_cmpkey);
        TEST_QSORT(TestWin_qsort_CompareStringW_null, win_cmp_null);
        TEST_QSORT(TestWin_qsort_CompareStringW_len, win_cmp_len);
        TEST_QSORT(TestWin_qsort_usekey, win_cmpkey);

#define TEST_BIN(testname, func)\
    TEST(testname, CmdBinSearch, col, win_langid, count, rnd_index, ord_icu_key, &CmdBinSearch::func)
        TEST_BIN(TestIcu_BinarySearch_strcoll_null, icu_strcoll_null);
        TEST_BIN(TestIcu_BinarySearch_strcoll_len, icu_strcoll_len);
        TEST_BIN(TestIcu_BinarySearch_usekey, icu_cmpkey);
        TEST_BIN(TestIcu_BinarySearch_strcmp, icu_strcmp);
        TEST_BIN(TestIcu_BinarySearch_cmpCPO, icu_cmpcpo);
        TEST_BIN(TestPosix_BinarySearch_strcoll_null, posix_strcoll_null);
        TEST_BIN(TestPosix_BinarySearch_usekey, posix_cmpkey);
        TEST_BIN(TestWin_BinarySearch_CompareStringW_null, win_cmp_null);
        TEST_BIN(TestWin_BinarySearch_CompareStringW_len, win_cmp_len);
        TEST_BIN(TestWin_BinarySearch_usekey, win_cmpkey);
        TEST_BIN(TestWin_BinarySearch_wcscmp, win_wcscmp);

        name="";
        return NULL;
    }



    void prepareData(UErrorCode& status){
        if(U_FAILURE(status)) return;
        if (icu_data) return; // prepared

        icu_data = new CA_uchar();

        // Following code is borrowed from UPerfTest::getLines();
        const UChar*    line=NULL;
        int32_t         len =0;
        for (;;) {
            line = ucbuf_readline(ucharBuf,&len,&status);
            if(line == NULL || U_FAILURE(status)){break;}

            // Refer to the source code of ucbuf_readline()
            // 1. 'len' includs the line terminal symbols
            // 2. The length of the line terminal symbols is only one character
            // 3. The Windows CR LF line terminal symbols will be converted to CR

            if (len == 1) {
                continue; //skip empty line
            } else {
                icu_data->append_one(len);
                memcpy(icu_data->last(), line, len * sizeof(UChar));
                icu_data->last()[len -1] = NULL;
            }
        }
        if(U_FAILURE(status)) return;

        // UTF-16 -> UTF-8 conversion.
        UConverter   *conv = ucnv_open("utf-8", &status); // just UTF-8 for now.
        if (U_FAILURE(status)) return;

        count = icu_data->count;

        icu_data_all_len =  icu_data->index[count]; // includes all NULLs
        icu_data_all_len -= count;  // excludes all NULLs
        icu_data_all_len += 1;      // the terminal NULL
        icu_data_all = new UChar[icu_data_all_len];
        icu_data_all[icu_data_all_len - 1] = 0; //the terminal NULL

        icu_key  = new CA_uint8;
        win_data = new CA_win_wchar;
        win_key  = new CA_char;
        posix_data = new CA_char;
        posix_key = new CA_char;
        rnd_index = new DataIndex[count];
        DataIndex::win_langid = win_langid;
        DataIndex::col        = col;


        UChar * p = icu_data_all;
        int32_t s;
        int32_t t;
        for (int i=0; i < count; i++) {
            // ICU all data
            s = sizeof(UChar) * icu_data->lengthOf(i);
            memcpy(p, icu_data->dataOf(i), s);
            p += icu_data->lengthOf(i);

            // ICU data

            // ICU key
            s = ucol_getSortKey(col, icu_data->dataOf(i), -1,NULL, 0);
            icu_key->append_one(s);
            t = ucol_getSortKey(col, icu_data->dataOf(i), -1,icu_key->last(), s);
            if (t != s) {status = U_INVALID_FORMAT_ERROR;return;}

            // POSIX data
            s = ucnv_fromUChars(conv,NULL, 0, icu_data->dataOf(i), icu_data->lengthOf(i), &status);
            if (status == U_BUFFER_OVERFLOW_ERROR || status == U_ZERO_ERROR){
                status = U_ZERO_ERROR;
            } else {
                return;
            }
            posix_data->append_one(s + 1); // plus terminal NULL
            t = ucnv_fromUChars(conv,posix_data->last(), s, icu_data->dataOf(i), icu_data->lengthOf(i), &status);
            if (U_FAILURE(status)) return;
            if ( t != s){status = U_INVALID_FORMAT_ERROR;return;}
            posix_data->last()[s] = 0;

            // POSIX key
            s = strxfrm(NULL, posix_data->dataOf(i), 0);
            if (s == INT_MAX){status = U_INVALID_FORMAT_ERROR;return;}
            posix_key->append_one(s);
            t = strxfrm(posix_key->last(), posix_data->dataOf(i), s);
            if (t != s) {status = U_INVALID_FORMAT_ERROR;return;}

            // Win data
            s = icu_data->lengthOf(i) + 1; // plus terminal NULL
            win_data->append_one(s);
            memcpy(win_data->last(), icu_data->dataOf(i), sizeof(WCHAR) * s);

            // Win key
            s = LCMapStringW(win_langid, LCMAP_SORTKEY, win_data->dataOf(i), win_data->lengthOf(i), NULL,0);
            if (s == 0) {status = U_INVALID_FORMAT_ERROR;return;}
            win_key->append_one(s);
            t = LCMapStringW(win_langid, LCMAP_SORTKEY, win_data->dataOf(i), win_data->lengthOf(i), (WCHAR *)(win_key->last()),s);
            if (t != s) {status = U_INVALID_FORMAT_ERROR;return;}

        };

        // append_one() will make points shifting, should not merge following code into previous iteration
        for (int i=0; i < count; i++) {
            rnd_index[i].icu_key = icu_key->dataOf(i);
            rnd_index[i].icu_data = icu_data->dataOf(i);
            rnd_index[i].icu_data_len = icu_data->lengthOf(i);
            rnd_index[i].posix_key = posix_key->last();
            rnd_index[i].posix_data = posix_data->dataOf(i);
            rnd_index[i].posix_data_len = posix_data->lengthOf(i);
            rnd_index[i].win_key = win_key->dataOf(i);
            rnd_index[i].win_data = win_data->dataOf(i);
            rnd_index[i].win_data_len = win_data->lengthOf(i);
        };

        ucnv_close(conv);
        qsort(rnd_index, count, sizeof(DataIndex), CmdQsort::q_random);

#define SORT(data, func) \
    data = new DataIndex[count];\
    memcpy(data, rnd_index, count * sizeof(DataIndex));\
    qsort(data, count, sizeof(DataIndex), CmdQsort::func)

        SORT(ord_icu_data, icu_strcoll_len);
        SORT(ord_icu_key, icu_cmpkey);
        SORT(ord_posix_data, posix_strcoll_null);
        SORT(ord_posix_key, posix_cmpkey);
        SORT(ord_win_data, win_cmp_len);
        SORT(ord_win_key, win_cmpkey);
        SORT(ord_win_wcscmp, win_wcscmp);
        SORT(ord_icu_strcmp, icu_strcmp);
        SORT(ord_icu_cmpcpo, icu_cmpcpo);
    }
};


int main(int argc, const char *argv[])
{

    UErrorCode status = U_ZERO_ERROR;
    CollPerfTest test(argc, argv, status);

    if (U_FAILURE(status)){
        printf("The error is %s\n", u_errorName(status));
        //TODO: print usage here
        return status;
    }

    if (test.run() == FALSE){
        fprintf(stderr, "FAILED: Tests could not be run please check the "
            "arguments.\n");
        return -1;
    }
    return 0;
}

