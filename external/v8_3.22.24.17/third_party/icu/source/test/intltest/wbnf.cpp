/*
 ******************************************************************************
 * Copyright (C) 2005-2007, International Business Machines Corporation and   *
 * others. All Rights Reserved.                                               *
 ******************************************************************************
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "wbnf.h"
    
// Most of this code is meant to test the test code. It's a self test.
// Normally this isn't run.
#define TEST_WBNF_TEST 0

///////////////////////////////////////////////////////////
//
// Constants and the most basic helper classes
//
    
static const char DIGIT_CHAR[] = "0123456789";
static const char WHITE_SPACE[] = {'\t', ' ', '\r', '\n', 0};
static const char ALPHABET[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const char SPECIAL[] = "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";

static inline UBool isInList(const char c /*in*/, const char list[] /*in*/){
    const char * p = list;
    for (;*p != 0 && *p != c; p++);
    return *p?TRUE:FALSE;
}
static inline UBool isDigit(char c) {return isInList(c, DIGIT_CHAR);}
static inline UBool isWhiteSpace(char c) {return isInList(c, WHITE_SPACE);}
static inline UBool isAlphabet(char c) {return isInList(c, ALPHABET);}
static inline UBool isSpecialAsciiChar(char c) {return isInList(c,SPECIAL);}

    

///////////////////////////////////////////////////////////
//
// Helper classes
//
    
class Buffer_byte{
// Utility class, can be treated as an auto expanded array. no boundary check.

    typedef char byte;
    byte * start;
    byte * current;
    int buffer_size; // size unit is byte
public:
    inline int content_size(){return current - start;} // size unit is byte

private:
    inline void expand(int add_size = 100){ // size unit is byte
        int new_size = buffer_size + add_size;

        int cs_snap = content_size();         
        start = (byte *) realloc(start, new_size);   // may change the value of start
        current = start + cs_snap;

        memset(current, 0, add_size);
        buffer_size = new_size;
    }

    inline void expand_to(int size){
        int r = size - buffer_size;
        if (r > 0) {
            expand(r);  // simply expand, no block alignment
        }
    }
    Buffer_byte(const Buffer_byte &);
    Buffer_byte & operator = (const Buffer_byte &);
public:
    Buffer_byte():start(NULL),current(start),buffer_size(0){
        expand();
    }
    ~Buffer_byte(){
        free(start);
    }

    inline void reset(){
        start != NULL ? memset(start, 0, buffer_size) : 0;
        current = start;
    }

    // Using memory copy method to append a C array to buffer, 
    inline void append(const void * c, int size){ // size unit is byte
        expand_to(content_size() + size) ;
        memcpy(current, c, size);
        current = current + size;
    }
    
    byte * buffer(){
        return start;
    }
};
    
/*
  The class(es) try to work as bulid-in array, so it overloads these two operators
    operator type *();
    type & operator[];
  The first is used to auto type convert, the latter is used to select member.

  A small trick is the class does not overload the address-of operator. This 
  behavior is different from bulid-in array, but it give us the opportunity 
  to get the address of the class itself.
*/
//template<typename type>
//    class BUFFER{
//       typedef BUFFER name;
#define BUFFER(type, name)\
    class name {\
    private:\
       Buffer_byte buf;\
    public:\
        name & reset() {buf.reset(); return *this;}\
        name & append(type c) {buf.append(&c, sizeof(type)); return *this;}\
        name & append_array(const type * p, int size) {buf.append(p, sizeof(type)*size); return *this;}\
        type & operator [] (int i) { return ((type *) buf.buffer())[i];}\
        operator type *(){return (type *) buf.buffer();} \
        int content_size(){return buf.content_size() / sizeof(type);}\
    }
    

class Pick{
/* The Pick is the basic language generator element*/
public:
    // generate a string accroding the syntax 
    // Return a null-terminated c-string. The buffer is owned by callee.
    virtual const char* next() = 0; 
    virtual ~Pick(){};
};

//typedef BUFFER<char> Buffer_char;
//typedef BUFFER<int> Buffer_int;
//typedef BUFFER<Pick *> Buffer_pPick;
BUFFER(char, Buffer_char);
BUFFER(int, Buffer_int);
BUFFER(Pick *, Buffer_pPick);

class SymbolTable{
/* Helper class.
* It's a mapping table between 'variable name' and its 'active Pick object'
*/
private:
    Buffer_char  name_buffer;   // var names storage space

    Buffer_int   names;         // points to name (offset in name_buffer)
    Buffer_pPick refs;          // points to Pick

    int get_index(const char *const var_name){
        int len = names.content_size();
        for (int i=0; i< len; i++){
            if (strcmp(var_name, name_buffer + names[i]) == 0){
                return i;
            }
        }
        return -1;
    }

public:
    enum RESULT {EMPTY, NO_VAR, NO_REF, HAS_REF};

    RESULT find(const char *const var_name /*[in] c-string*/, Pick * * ref = NULL /*[out] Pick* */){
        if (!var_name) return EMPTY; // NULL name

        int i = get_index(var_name);
        if (i == -1){
            return NO_VAR;   // new name
        }
        if (!refs[i]){  // exist name, no ref
            return NO_REF;
        } else {
            if (ref) {
                *ref = refs[i];
            }
            return HAS_REF;   // exist name, has ref
        }
    }

    void put(const char *const var_name, Pick *const var_ref = NULL){
        int i = get_index(var_name);
        switch(find(var_name)){
            case EMPTY:    // NULL name
                break;
            case NO_VAR:    // new name
                int offset;
                offset = name_buffer.content_size();
                name_buffer.append_array(var_name, strlen(var_name) + 1);
                names.append(offset);
                refs.append(var_ref);
                break;
            case NO_REF:    // exist name, no ref
                refs[i] = var_ref;    // link definition with variable
                break;
            case HAS_REF:    // exist name, has ref
                if (var_ref){
                    refs[i] = var_ref;
                }
                break;
            default:
                ; // ASSERT(FALSE);
        }
        return;
    }

    UBool is_complete(){
        int n = names.content_size();
        for (int i=0; i<n; ++i){
            if (refs[i] == NULL){
                return FALSE;
            }
        }
        return TRUE;
    }

    void reset(){
        names.reset();
        name_buffer.reset();

        // release memory here
        int s = refs.content_size();
        for (int i=0; i < s; i++){
            delete refs[i]; // TOFIX: point alias/recursion problem
        }
        refs.reset();
    }

    ~SymbolTable(){
        reset();
    }
};


/*
// Document of class Escaper
//
// ATTENTION: 
// From http://icu-project.org/userguide/Collate_Customization.html.
// We get the precedence of escape/quote operations
//
//     (highest) 1. backslash               \
//               2. two single quotes       ''
//               3. quoting                 ' '
//
// ICU Collation should accept following as the same string.
//
// 1)  'ab'c        _
// 2)  a\bc          \
// 3)  a'b'\c        |- They are equal.
// 4)  abc          _/
//
// From "two single quotes", we have following deductions
//    D1. empty quoting is illgal. (obviously)
//    D2. no contact operation between two quotings   
//              '.''.'      is not ..   it is .'.
//    D3. "two single quotes" cannot contact two quoting simultaneously
//              '..''''.'   is not ..'. it is ..''.
//       NOTICE:
//        "two single quotes" can contact before one quoting
//              '''.'       is '.
//        "two single quotes" can literally contact after one quoting
//        But, from syntax, it's one quoting including a "two single quotes"
//              '.'''       is .'
//    D4. "two single quotes" cannot solely be included in quoting
//              ''''        is not '    it is ''
//       NOTICE:  These are legal
//              '.''.'      is .'.
//              '.'''       is .'
//
//                 dicision
//                    /\
//                   /__\
//      output buffer    input buffer
// 
// To make our dicision (within an atom operation) without caring input and output buffer,
// following calling pattern (within an atom operation) shall be avoided
//
//    P1 open_quoting()  then close_quoting()    (direct violation)   D1
//    P2 close_quoting() then open_quoting()     (direct violation)   D2
//    P3 empty open_quoting()                    (indirect violation) D1, D4
//    P4 empty close_quoting()                   (indirect violation) D2, D3
//    P5 open_quoting()  then two single quotes  (indirect violation) D4
//    P6 close_quoting() then two single quotes  (indirect violation) D3
//
// two single quotes escaping will not open_ or close_ quoting()
// The choice will not lose some quoing forms.
//
// For open_quoting(), 
// we may get this form quoting     '''         P5
// It may raise a bug               ''''x
// If we expect
//      '''.'       let the next char open the quoting
//      '.''.'      the quoting is already opened by preceding char
//
// For close_quoting()
// we will get this form quoting    '.'''       P6
// It may raise a bug               '.''''.'
// If we expect          
//      '.'''\.     let the next char close the quoting
//      '.''''.'    the expectation is wrong!  using  '.'\''.' instead
//
// It's a hard work to re-adjust generation opportunity for various escaping form.
// We just simply ignore it.
*/
class Escaper{
public:
    enum CHOICE {YES, NO, RAND};
    enum ESCAPE_FORM {BSLASH_ONLY, QUOTE_ONLY, QUOTE_AND_BSLAH, RAND_ESC};
private:
    class Bool{ // A wrapper class for CHOICE, to auto adapter UBool class
        private:
            const CHOICE tag;
        public:
            Bool(CHOICE flag=RAND):tag(flag){}
            operator UBool() {   // conversion operator
                return tag == RAND ? rand()%2 : tag == YES;
                //if (tag == RAND){
                //    return rand()%2 == 1;
                //} else {
                //    return tag == YES ? TRUE : FALSE;
                //}
            }
    };
public:
    Escaper(CHOICE escapeLiteral = RAND,
        CHOICE twoQuotesEscape = RAND,
        ESCAPE_FORM escapeForm = RAND_ESC):
        escape_form(escapeForm),
        escape_literal(escapeLiteral),
        two_quotes_escape(twoQuotesEscape),
        is_quoting(FALSE){}
private:
    Buffer_char str;
    ESCAPE_FORM escape_form;
    Bool escape_literal;
    Bool two_quotes_escape;
    UBool quote_escape;
    UBool bslash_escape;
    UBool is_quoting;

    void set_options(){
        ESCAPE_FORM t = escape_form == RAND_ESC ? (ESCAPE_FORM) (rand()%3) : escape_form;
        switch (t){
                case BSLASH_ONLY :
                    bslash_escape = TRUE; quote_escape = FALSE; break;
                case QUOTE_ONLY:
                    bslash_escape = FALSE;quote_escape = TRUE;  break;
                case QUOTE_AND_BSLAH:
                    bslash_escape = TRUE; quote_escape = TRUE;  break;
                default:
                    ;// error
        }
    }

    void reset(){
        str.reset();
        is_quoting = FALSE;
    }

    inline void open_quoting(){ 
        if(is_quoting){
            // do nothing
        } else {
            str.append('\'');
            is_quoting = TRUE;
        }
    }
    inline void close_quoting(){
        if(is_quoting){
            str.append('\'');
            is_quoting = FALSE;
        } else {
            // do nothing
        }
    }

    // str  [in]    null-terminated c-string
    void append(const char * strToAppend){
        for(;*strToAppend != 0; strToAppend++){
            append(*strToAppend);
        }
    }

    inline void append(const char c){
        set_options();

        if (c == '\\'){
            quote_escape ? open_quoting() : close_quoting();
            //bslash_escape always true here
            str.append('\\');
            str.append('\\');
        } else if (c == '\''){
            if (two_quotes_escape){     // quoted using two single quotes
                // See documents in anonymous.design
                str.append('\'');
                str.append('\'');
            } else{
                quote_escape ? open_quoting() : close_quoting();
                //bslash_escape always true here
                str.append('\\');
                str.append('\'');
            }
        } else if (isSpecialAsciiChar(c) || isWhiteSpace(c)){
            quote_escape  ? open_quoting()   : close_quoting();
            if (bslash_escape) str.append('\\');
            str.append(c);
        } else { //if (isAlphabet(c) || isDigit(c) || TRUE){ // treat others as literal
            if (escape_literal){
                quote_escape  ? open_quoting()   : close_quoting();
                if (bslash_escape)  str.append('\\');
                str.append(c);
            } else {
                close_quoting();
                str.append(c);
            }
        }
    }

public:
    // Return a null-terminate c-string. The buffer is owned by callee.
    char * operator()(const char * literal /*c-string*/){
        str.reset();
        for(;*literal != 0; literal++){
            append(*literal);
        }
        close_quoting();    // P4 exception, to close whole quoting
        return str;
    }
};
    
class WeightedRand{
// Return a random number in [0, size)
// Every number has different chance (aka weight) to be selected.
private:
    Buffer_int weights;
    double total;
    WeightedRand(const WeightedRand &);
    WeightedRand & operator = (const WeightedRand &);
public:
    WeightedRand(Buffer_int * weight_list = NULL, int size = 0){
        if ( weight_list == NULL){
            for (int i=0; i<size; ++i) weights.append(DEFAULT_WEIGHT);
        } else {
            int s = weight_list->content_size();
            if (s < size){
                weights.append_array( (*weight_list),s);
                for (int i=s; i<size; ++i) weights.append(DEFAULT_WEIGHT);
            } else { // s >= size
                weights.append_array( (*weight_list),size);
            }
        }
        total = 0;
        int c = weights.content_size();
        for (int i=0; i<c; ++i){
            total += weights[i];
        }
    }

    void append(int weight){
        weights.append(weight);
        total += weight;
    }

    // Give a random number with the consideration of weight.
    // Every random number is associated with a weight.
    // It identifies the chance to be selected, 
    // larger weight has more chance to be selected.
    //
    //
    //  ______________________   every slot has equal chance
    //
    //  [____][_][___][______]   each item has different slots, hence different chance
    //
    //
    //  The algorithms to generate the number is illustrated by preceding figure.
    //  First, a slot is selected by rand(). Then we translate the slot to corresponding item.
    //
    int next(){
        // get a random in [0,1]
        double reference_mark = (double)rand() / (double)RAND_MAX;

        // get the slot's index, 0 <= mark <= total;
        double mark = total * reference_mark;

        // translate the slot to corresponding item
        int i=0;
        for (;;){
            mark -= weights[i];  // 0 <= mark <= total
            if (mark <= 0)
                break;
            i++;
        }
        return i;
    }
};
    
///////////////////////////////////////////////////////////
//
// The parser result nodes
//
    
class Literal : public Pick {
public:
    virtual const char* next(){
        return str;
    }
    Literal(const char * s /*c-string*/){
        str.append_array(s, strlen(s) + 1);
    }
private:
    Buffer_char str; //null-terminated c-string
};

class Variable : public Pick {
public:
    Variable(SymbolTable * symbols, const char * varName, Pick * varRef = NULL){
        this->var_name.append_array(varName, strlen(varName) + 1);
        if ((symbol_table = symbols)){
            symbol_table->put(varName, varRef);
        }
    }

    operator const char *(){
        return var_name;
    }

    virtual const char* next(){
        if (symbol_table){
            Pick * var_ref = NULL;
            symbol_table->find(var_name, &var_ref);
            if (var_ref) {
                return var_ref->next();
            }
        }
        return "";  // dumb string
    }
private:
    Buffer_char var_name;
    SymbolTable * symbol_table;
};

class Quote : public Pick{
public:
    Quote(Pick & base):item(base),e(Escaper::NO, Escaper::NO, Escaper::BSLASH_ONLY){
    }
    virtual const char* next(){
        return e(item.next());
    }
private:
    Pick & item;
    Buffer_char str;
    Escaper e;
};


class Morph : public Pick{
/*
The difference between morph and an arbitrary random string is that 
a morph changes slowly. When we build collation rules, for example, 
it is a much better test if the strings we use are all in the same 
'neighborhood'; they share many common characters.
*/
public:
    Morph(Pick & base):item(base){}

    virtual const char* next(){
        current.reset();
        const char * s = item.next();
        current.append_array(s, strlen(s) + 1);
        if  (last.content_size() == 0) {
            str.reset();
            last.reset();
            str.append_array(current, current.content_size());
            last.append_array(current, current.content_size());
        } else {
            morph();
        }
        return str;
    }
private:
    Pick & item;
    Buffer_char str;
    Buffer_char last;
    Buffer_char current;

    char * p_last;
    char * p_curr;

    void copy_curr(){
        if (*p_curr) {
            str.append(*p_curr);
            p_curr++;
        }
    }

    void copy_last(){
        if (*p_last) {
            str.append(*p_last);
            p_last++;
        }
    }

    // copy 0, 1, or 2 character(s) to str
    void copy(){
        static WeightedRand wr(& Buffer_int().append(DEFAULT_WEIGHT * 10), 5);

        switch (wr.next()){
            case 0: // copy last  -- has 10 times chance than others
                copy_last();
                break;
            case 1: // copy both
                copy_curr();
                copy_last();
                break;
            case 2: // copy both
                copy_last();
                copy_curr();
                break;
            case 3:
                copy_curr();
                break;
            case 4:  // copy nothing
                break;
            default:
                // ASSERT(FALSE);
                ;
        }
    }

    void morph(void){
        int min = strlen(last);
        int max = strlen(current);
        if (min > max){
            int temp  = min;
            min = max;
            max = temp;
        }

        int len = min + rand()%(max - min + 1); // min + [0, diff]
        p_curr = current;
        p_last = last;
        str.reset();

        for (; str.content_size()<len && *p_curr && *p_last;){
            copy(); // copy 0, 1, or 2 character(s) to str
        }

        if (str.content_size() == len) {
            str.append(0);
            final();
            return;
        }

        if (str.content_size() > len) { // if the last copy copied two characters
            str[len]=0;
            final();
            return;
        }

        // str.content_size() < len
        if (*p_last) {
            for (; str.content_size() < len; copy_last());
        } else if (*p_curr){
            for (; str.content_size() < len; copy_curr());
        }

        int last_len = last.content_size();
        for (;str.content_size() < len;){
            str.append(last[rand()%last_len]);
        }
        str.append(0);
        final();
    }

    void final(){
        last.reset();
        last.append_array(current, current.content_size());
    }
};

class Sequence : public Pick {
public:
    virtual const char* next(){
        str.reset();
        int s = items.content_size();
        for(int i=0; i < s; i++){
            const char * t = items[i]->next();
            str.append_array(t, strlen(t));
        }
        str.append(0); // terminal null
        return str;
    }

    void append (Pick * node){
        items.append(node);
    }

    virtual ~Sequence(){
        int s = items.content_size();
        for(int i=0; i < s; i++){
            //How can assure the item is got from heap?
            //Let's assume it.
            delete items[i]; // TOFIX: point alias/recursion problem
            items[i] = NULL;
        }
    }
private:
    Buffer_pPick items;
    Buffer_char  str; //null-terminated c-string
};

class Repeat : public Pick {
private:
    Pick * item;
    Buffer_char str;
    WeightedRand wr;
    int min;
    int max;
    int select_a_count(){
        return min + wr.next();
    }
public:
    virtual const char* next(){
        str.reset();
        int c = select_a_count();
        for(int i=0; i< c; i++){
            const char * t = item->next();
            str.append_array(t, strlen(t));
        }
        str.append(0);
        return str;
    }

    Repeat(Pick * base, int minCount =0, int maxCount = 1, Buffer_int * weights = NULL):
        wr(weights, maxCount-minCount +1) {
        this->item = base;
        this->min = minCount;
        this->max = maxCount;
    }
    virtual ~Repeat(){
        delete item;  // TOFIX: point alias/recursion problem
        item = NULL;
    }
};


class Alternation : public Pick {
public:
    virtual const char* next(){
        str.reset();
        int i = wr.next();
        const char * t = items[i]->next();
        str.append_array(t, strlen(t) + 1);
        return str;
    }
    virtual ~Alternation(){
        int s = items.content_size();
        for(int i=0; i < s; i++){
            delete items[i];  // TOFIX: point alias/recursion problem
            items[i] = NULL;
        }
    }

    Alternation & append (Pick * node, int weight = DEFAULT_WEIGHT){
        items.append(node);
        wr.append(weight);
        return *this;
    }
private:
    Buffer_pPick items;
    Buffer_char str; // null-terminated c-string
    WeightedRand wr;
};
    
///////////////////////////////////////////////////////////
//
// The parser
//

enum TokenType {STRING, VAR, NUMBER, STREAM_END, ERROR, QUESTION, STAR, PLUS, LBRACE, RBRACE, LPAR, RPAR, SEMI, EQ, COMMA, BAR, AT, WAVE, PERCENT};

class Scanner{
friend int DumpScanner(Scanner & s, UBool dumb);
private:
    const char * source;
    const char * working;
    const char * history; // for debug
    enum StateType {START, IN_NUM, IN_VAR_FIRST, IN_VAR, IN_QUOTE, IN_QUOTE_BSLASH, IN_BSLASH, IN_STRING, DONE};
    StateType state;
    void terminated(TokenType t){
        working--;       // return the peeked character
        tokenType = t;
        token.append(0); // close buffer
        state = DONE;
    }
public:
    // the buffer of "source" is owned by caller
    Scanner(const char *src/*[in] c-string*/ = NULL):source(src){
        working = src;
        history = working;
        state = DONE;
        tokenType = ERROR;
    }

    //void setSource(const char *const src /*[in] c-string*/){
    //    *(&const_cast<const char *>(source)) = src;
    //}

    Buffer_char token;
    TokenType tokenType;

    TokenType getNextToken(){
        token.reset();
        state = START;
        history = working; // for debug
        while (state != DONE){
            char c = *working++;
            if (c == 0 && state != START){//avoid buffer overflow. for IN_QUOE, IN_ESCAPE
                terminated(ERROR);
                break; // while
            }
            switch(state){
                case START:
                    tokenType = ERROR;
                    switch(c){
                        case '?'  : tokenType = QUESTION; break;
                        case '*'  : tokenType = STAR; break;
                        case '+'  : tokenType = PLUS; break;
                        case '{'  : tokenType = LBRACE; break;
                        case '}'  : tokenType = RBRACE; break;
                        case '('  : tokenType = LPAR; break;
                        case ')'  : tokenType = RPAR; break;
                        case ';'  : tokenType = SEMI; break;
                        case '='  : tokenType = EQ; break;
                        case ','  : tokenType = COMMA; break;
                        case '|'  : tokenType = BAR; break;
                        case '@'  : tokenType = AT; break;
                        case '~'  : tokenType = WAVE; break;
                        case '%'  : tokenType = PERCENT; break;
                        case 0    : tokenType = STREAM_END; working-- /*avoid buffer overflow*/; break;
                    }
                    if (tokenType != ERROR){
                        token.append(c);
                        token.append(0);
                        state = DONE;
                        break; // START
                    }
                    switch(c){
                        case '$'  : state = IN_VAR_FIRST; token.append(c); break;
                        case '\'' : state = IN_QUOTE;     break;
                        case '\\' : state = IN_BSLASH;    break;
                        default:
                            if (isWhiteSpace(c)){    // state = START;   //do nothing
                            } else if (isDigit(c)){     state = IN_NUM;    token.append(c);
                            } else if (isAlphabet(c)){  state = IN_STRING; token.append(c);
                            } else {terminated(ERROR);}
                    }
                    break;//START
                case IN_NUM:
                    if (isDigit(c)){
                        token.append(c);
                    } else {
                        terminated(NUMBER);
                    }
                    break;//IN_NUM
                case IN_VAR_FIRST:
                    if (isAlphabet(c)){
                        token.append(c);
                        state = IN_VAR;
                    } else {
                        terminated(ERROR);
                    }
                    break; // IN_VAR_FISRT
                case IN_VAR:
                    if (isAlphabet(c) || isDigit(c)){
                        token.append(c);
                    } else {
                        terminated(VAR);
                    }
                    break;//IN_VAR
                case IN_STRING:
                    // About the scanner's behavior for STRING, AT, and ESCAPE:
                    // All of them can be contacted with each other. 
                    // This means the scanner will eat up as much as possible strings
                    //   (STRING, AT, and ESCAPE) at one time, with no regard of their
                    //   combining sequence.
                    //
                    if (c == '\''){
                        state = IN_QUOTE; // the first time we see single quote
                    } else if (c =='\\'){ // back slash character
                        state = IN_BSLASH;
                    } else if (isAlphabet(c) || isDigit(c)){
                        token.append(c);
                    } else{
                        terminated(STRING);
                    }
                    break;//IN_STRING
                case IN_QUOTE:
                    if (c == '\''){ // the second time we see single quote
                        state = IN_STRING; // see document in IN_STRING
                    } else if ( c== '\\') { // backslah escape in quote
                        state = IN_QUOTE_BSLASH;
                    } else {
                        token.append(c);  // eat up everything, includes back slash
                    }
                    break;//IN_QUOTE
                case IN_QUOTE_BSLASH:
                case IN_BSLASH:
                    switch (c){
                        case 'n'  : token.append('\n'); break;
                        case 'r'  : token.append('\r'); break;
                        case 't'  : token.append('\t'); break;
                        case '\'' : token.append('\''); break;
                        case '\\' : token.append('\\'); break;
                        default: token.append(c); // unknown escaping, treat it as literal
                    }
                    if (state == IN_BSLASH){
                        state = IN_STRING; // see document in IN_STRING
                    } else { // state == IN_QUOTE_BSLASH
                        state = IN_QUOTE;
                    }
                    break;//IN_BSLASH
                case DONE:  /* should never happen */
                default:
                    working--;
                    tokenType = ERROR;
                    state = DONE;
                    break;
            }//switch(state) 
        }//while (state != DONE)

        return tokenType;
    }
};//class Scanner

class Parser{
friend UBool TestParser();
friend class TestParserT;
friend class LanguageGenerator_impl;
private:
    Scanner s;
    TokenType & token;
    int min_max;   // for the evil infinite
    
    UBool match(TokenType expected){
        if (token == expected) {
            token = s.getNextToken();
            return TRUE;
        } else {
            //s.dumpCurrentPoint();
            return FALSE;
        }
    }

    UBool weight(int & value){
        if (token == NUMBER){
            int temp = atoi(s.token);
            match(NUMBER);
            if (match(PERCENT)){
                value = temp;
                return TRUE;
            }
        }
        return FALSE;
    }

    UBool repeat (Pick* &node /*in,out*/){
        if (node == NULL) return FALSE;

        int count = -2;
        int min = -2;
        int max = -2;
        UBool question = FALSE;
        switch (token){
            case QUESTION:
                match(QUESTION);
                min = 0;
                max = 1;
                count = 2;
                question = TRUE;
                break;
            case STAR:
                match(STAR);
                min = 0;
                max = -1;
                count = -1;
                break;
            case PLUS:
                match(PLUS);
                min = 1;
                max = -1;
                count = -1;
                break;
            case LBRACE:
                match(LBRACE);
                if (token != NUMBER){
                    return FALSE;
                }else {
                    min = atoi(s.token);
                    match(NUMBER);
                    if (token == RBRACE){
                        match(RBRACE);
                        max = min;
                        count = 1;
                    } else if (token == COMMA) {
                        match(COMMA);
                        if (token == RBRACE){
                            match(RBRACE);
                            max = -1;
                            count = -1;
                        } else if (token == NUMBER) {
                            max = atoi(s.token);
                            match(NUMBER);
                            count = max - min + 1;
                            if (!match(RBRACE)) {
                                return FALSE;
                            }
                        } else {
                            return FALSE;
                        }
                    } else {
                        return FALSE;
                    }
                }
                break;
            default:
                return FALSE;
        }

        if (count == -2 || min == -2 || max == -2){
            //ASSERT(FALSE);
            return FALSE;
        }

        // eat up following weights
        Buffer_int weights;
        int w;
        while (weight(w)){
            weights.append(w);
        }

        // for the evil infinite
        min_max = min_max > min ? min_max : min;
        min_max = min_max > max ? min_max : max;
        if (min_max > PSEUDO_INFINIT){
            return FALSE; // PSEUDO_INFINIT is less than the real maximum
        }
        if (max == -1){ // the evil infinite
            max = PSEUDO_INFINIT;
        }
        // for the strange question mark
        if (question && weights.content_size() > 0){
            Buffer_int w2;
            w2.append(DEFAULT_WEIGHT - weights[0]).append(weights[0]);
            node = new Repeat(node,min,max,&w2);
            return TRUE;
        }
        node = new Repeat(node,min,max,&weights);
        return TRUE;
    }

    UBool core(Pick* &node /*out*/){
        if (node != NULL) return FALSE; //assert node == NULL

        switch(token){
            case LPAR:
                match(LPAR);
                if(defination(node) && match(RPAR)){
                    return TRUE;
                }
                return FALSE;
            case VAR:
                node = new Variable(&symbols, s.token);
                match(VAR);
                return TRUE;
            case STRING:
                node = new Literal(s.token);
                match(STRING);
                return TRUE;
            default:
                return FALSE;
        }
    }
    UBool modified(Pick* &node /*out*/){
        if (node != NULL) return FALSE; //assert node == NULL

        if (!core(node)) {
            return FALSE;
        }

        for (;;){
            switch(token){
                case WAVE:
                    match(WAVE);
                    node = new Morph(*node);
                    break;
                case AT:
                    match(AT);
                    node = new Quote(*node);
                    break;
                case QUESTION:
                case STAR:
                case PLUS:
                case LBRACE:
                    if (!repeat(node)) return FALSE;
                    break;
                case SEMI:      // rule definiation closed
                case RPAR:      // within parenthesis (core closed)
                case BAR:       // in alternation
                case NUMBER:    // in alternation, with weight
                case LPAR:      // in sequence
                case VAR:       // in sequence
                case STRING:    // in sequence
                    return TRUE;
                default:
                    return FALSE;
            }
        }
    }


    UBool sequence_list(Pick* &node /*in,out*/){
        if (node == NULL) return FALSE; // assert node != NULL

        Sequence* seq = new Sequence();
        Pick * n = node;

        while (token == VAR || token == STRING || token == LPAR){
            seq->append(n);
            n = NULL;
            if (modified(n)){
                // go on
            } else {
                goto FAIL;
            }
        }

        if (token == SEMI || token == RPAR || token == BAR){
            seq->append(n);
            node = seq;
            return TRUE;
        }
FAIL:
        delete seq;
        return FALSE;

    }

    UBool sequence(Pick* &node /*out*/){
        if (node != NULL) return FALSE; //assert node == NULL

        if (!modified(node)) {
            return FALSE;
        }

        if (token == VAR || token == STRING || token == LPAR){
            return sequence_list(node);
        } else {
            return TRUE; // just a modified
        }
    }

    UBool alternation_list(Pick* &node /*in,out*/){
        if (node == NULL) return FALSE; // assert node != NULL

        Alternation * alt = new Alternation();
        Pick * n = node;
        int w = DEFAULT_WEIGHT;

        while (token == NUMBER || token == BAR){
            if(token == NUMBER) {
                if (weight(w)){ 
                    if (token == BAR){ 
                        // the middle item, go on
                    } else {
                        // the last item or encounter error
                        break; //while
                    } 
                } else {
                    goto FAIL;
                }
            } // else token == BAR
            match(BAR);
            alt->append(n,w);

            n = NULL;
            w = DEFAULT_WEIGHT;
            if (sequence(n)){
                // go on
            } else {
                goto FAIL;
            }
        }

        if (token == SEMI || token == RPAR) {
            alt->append(n,w);
            node = alt;
            return TRUE;
        }
FAIL:
        delete alt;
        return FALSE;
    }

    UBool alternation(Pick* &node /*out*/){
        if (node != NULL) return FALSE; //assert node == NULL

        // 'sequence' has higher precedence than 'alternation'
        if (!sequence(node)){
            return FALSE;
        }

        if (token == BAR || token == NUMBER){ // find a real alternation1, create it.
            return alternation_list(node);
        } else {
            return TRUE;    // just a sequence_old
        }
    }


    UBool defination(Pick* &node /*out*/){
        if (node != NULL) return FALSE; //assert node == NULL
        return alternation(node);
    }

    UBool rule(){
        if (token == VAR){
            Buffer_char name;
            name.append_array(s.token, strlen(s.token) + 1);
            match(VAR);

            if (match(EQ)){
                Pick * t = NULL;
                if(defination(t)){
                    symbols.put(name, t);
                    return match(SEMI);
                }
            }
        }
        return FALSE;
    }
public:
    UBool rules(){
        symbols.reset();
        token = s.getNextToken();
        while (rule()){
        }
        if (token == STREAM_END){
            return TRUE;
        } else {
            //s.dumpCurrentPoint();
            return FALSE;
        }
    }

public:
    SymbolTable symbols;

    Parser(const char *const source):s(source), token(s.tokenType){
        min_max = -2;
    }
    UBool parse(){
        return rules();
    }

}; // class Parser
        

///////////////////////////////////////////////////////////
//
// 
//

int DumpScanner(Scanner & s, UBool dump = TRUE){
    int len = strlen(s.source);
    int error_start_offset = s.history - s.source;
    if (dump){
        printf("\n=================== DumpScanner ================\n");
        fwrite(s.source, len, 1, stdout);
        printf("\n-----parsed-------------------------------------\n");
        fwrite(s.source, s.history - s.source, 1, stdout);
        printf("\n-----current------------------------------------\n");
        fwrite(s.history, s.working - s.history, 1, stdout);
        printf("\n-----unparsed-----------------------------------\n");
        fwrite(s.working, (s.source + len - s.working), 1, stdout);
        printf("\n^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
    }
    return error_start_offset;
}

class LanguageGenerator_impl{
public:
    LanguageGenerator_impl(const char *const bnf_definition, const char *const top_node)
        :par(bnf_definition), top_node_name(top_node){
        srand((unsigned)time( NULL ));
    }

    LanguageGenerator::PARSE_RESULT parseBNF(UBool debug = TRUE){
        if (par.parse()){
            if (par.symbols.find(top_node_name, &top_node_ref) == SymbolTable::HAS_REF) {
                if (par.symbols.is_complete()) {
                    return LanguageGenerator::OK;
                } else {
                    if (debug) printf("The bnf definition is incomplete.\n");
                    return LanguageGenerator::INCOMPLETE;
                }
            } else {
                if (debug) printf("No top node is found.\n");
                return LanguageGenerator::NO_TOP_NODE;
            }
        } else {
            if(debug) {
                printf("The bnf definition is wrong\n");
                DumpScanner(par.s, TRUE);
            }
            return LanguageGenerator::BNF_DEF_WRONG;
        }
    }
    const char * next(){
        return top_node_ref->next();
    }

private:
    Parser par;
    const char *const top_node_name;
    Pick * top_node_ref;
};

LanguageGenerator::LanguageGenerator():lang_gen(NULL){
}

LanguageGenerator::~LanguageGenerator(){
    delete lang_gen;
}

LanguageGenerator::PARSE_RESULT LanguageGenerator::parseBNF(const char *const bnf_definition /*in*/, const char *const top_node/*in*/, UBool debug){
    if (lang_gen){
        delete lang_gen;
    }
    lang_gen = new LanguageGenerator_impl(bnf_definition, top_node);
    PARSE_RESULT r = lang_gen->parseBNF(debug);
    if (r != OK){
        delete lang_gen;
        lang_gen = NULL;
        return r;
    } else {
        return r;
    }
}
const char *LanguageGenerator::next(){ // Return a null-terminated c-string. The buffer is owned by callee.
    if (lang_gen){
        return lang_gen->next();
    }else { 
        return "";
    }
}

///////////////////////////////////////////////////////////
//
// The test code for WBNF
//

#define CALL(fun) \
    if (fun()){ \
        printf("Pass: " #fun "\n");\
    } else { \
        printf("FAILED: !!! " #fun " !!!\n"); \
    }

#define DUMP_R(fun, var, times) \
    {printf("\n========= " #fun " =============\n"); \
    for (int i=0; i<times; i++) { \
        const char * t = var.next();\
        fwrite(t,strlen(t),1,stdout); \
        printf("\n");   \
    }   \
    printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");}



#if TEST_WBNF_TEST    
static UBool TestQuote(){
    const char *const str = "This ' A !,z| qq [] .new\tline";
    //const char *const str_r = "This \\' A '!,'z'|' qq '[]' '.'new\tline";
    ////
    //// :(  we must quote our string to following C syntax
    ////     cannot type the literal here, it makes our code rather human unreadable
    ////     very very unconformable!
    ////
    ///* 
    //*/

    //const char *const s1    =   "ab'c";
    //const char (* s1_r1) [] = { "ab''c",    // ab''c
    //                            "ab\\'c",   // ab\'c
    //                           };//
    ///*
    // .      '.'     \.
    // ..     \.\.    '.'\.   '.'\.   '..'    // '.''.'  wrong
    //*/

    //const char *const s2    =   "a..'.b";       // a..'.b
    //const char (*s2_r) []   = { "a'..''.'b"     // a'..''.'b   
    //                           ,"a'..\\'.'b"    // a'..\'.'b
    //                           ,"a'..'\\''.'b"  // a'..'\''.'b
    //                          };//

    //const char *const s3    =   "a..\\.b";      // a..\.b
    //const char (*s3_r) []   = { "a'..\\\\.'b"   // a'..\\.'b
    //                           ,"a'..'\\\\'.'b" // a'..'\\'.'b
    //                          };//

    //                            // no catact operation, no choice, must be compact

    srand((unsigned)time( NULL ));
    
    //Escaper l(Escaper::NO, Escaper::NO, Escaper::RAND_ESC);
    Pick *p = new Literal(str);
    Quote q(*p);

    DUMP_R(TestQuote, (*p), 1);
    DUMP_R(TestQuote, q, 20);
    return FALSE;
}
static UBool TestLiteral(){
    const char * s = "test string99.";
    Literal n(s);
    const char * r = n.next();
    return strcmp(s,r) == 0;
}

static UBool TestSequence(){
    Sequence seq;
    seq.append(new Literal("abc "));
    seq.append(new Literal(", s"));

    return strcmp(seq.next(), "abc , s") == 0;
}
static UBool TestAlternation(){
    srand((unsigned)time( NULL ));
    Alternation alt;
    alt.append(new Literal("aaa_10%"),10);
    alt.append(new Literal("bbb_0%"),0);
    alt.append(new Literal("ccc_10%"),10);
    alt.append(new Literal("ddddddd_50%"),50);

    DUMP_R(TestAlternation, alt, 50);

    return FALSE;
}

static UBool TestBuffer(){
    Buffer_int t;
    t.append(1).append(0).append(5);
    int s = t.content_size();
    for (int i=0; i<s; ++i){
        printf("%d\n", t[i]);
    }
    return FALSE;
}

static UBool TestWeightedRand(){
    srand((unsigned)time( NULL ));
    Buffer_int t;
    t.append(1).append(0).append(5);
    WeightedRand wr(&Buffer_int().append(10).append(0).append(50),4);
//    WeightedRand wr(&t,3);
    for (int i=0; i< 50; ++i){
        printf("%d\n", wr.next());
    }
    return FALSE;
}

static UBool TestRepeat(){
    srand((unsigned)time( NULL ));
    Repeat rep(new Literal("aaa1-5 "), 1, 5);
    DUMP_R(TestRepeat, rep, 50);

    Repeat r2(new Literal("b{1,3}1%0%5% "), 1, 3, &Buffer_int().append(1).append(0).append(5));
    DUMP_R(TestRepeat, r2, 50);

    Repeat r3(new Literal("aaa5-5 "), 5, 5);
    DUMP_R(TestRepeat, r3, 50);

    return FALSE;
}

static UBool TestVariable(){
    SymbolTable tab;
    Pick * value = new Literal("string1");
    Variable var1(&tab, "x", value);

    Variable var2(&tab, "y");
//    tab.put(var2, value); // TOFIX: point alias/recursion problem
    Pick * value2 = new Literal("string2");
    tab.put(var2, value2);

    Pick * value3 = new Literal("string3");
    Variable var3(&tab, "z");
    tab.put("z", value3);

    UBool pass;
    pass = strcmp(var1.next(), value->next()) == 0;
    pass = pass && strcmp(var2.next(), value2->next()) == 0;
    pass = pass && strcmp(var3.next(), value3->next()) == 0;
    return pass;
}

static UBool TestSymbolTable(){
    Literal * n1 = new Literal("string1");
    Literal * n2 = new Literal("string2");
    SymbolTable t;
    t.put("abc", n1);
    t.put("$aaa", n2);
//    t.put("alias", n1);  // TOFIX: point alias/recursion problem
    t.put("bbb");

    UBool pass;
    pass = t.find(NULL) == SymbolTable::EMPTY;
    pass = pass && t.find("ccc") == SymbolTable::NO_VAR;
    pass = pass && t.find("bbb") == SymbolTable::NO_REF;
    pass = pass && t.find("abc") == SymbolTable::HAS_REF;
    pass = pass && t.find("$aaa") == SymbolTable::HAS_REF;

    t.reset();
    pass = pass && t.find("abc") == SymbolTable::NO_VAR;
    return pass;
}


static UBool TestScanner(void){
    //const char str1[] = "$root = $command{0,5} $reset $mostRules{1,20};";
    //const char str1_r[][20] = {"$root", "=", "$command", "{", "0", ",", "5", "}", 
    //    "$reset", "$mostRules", "{", "1", ",", "20", "}", ";"};

    const char str2[] = "$p2 =(\\\\ $s $string $s)? 25%;";
    const char str2_r[][20] = {"$p2", "=", "(", "\\", "$s", "$string", "$s", ")", "?", "25", "%", ";"};

    const char *str = str2;
    const char (*str_r)[20] = str2_r;
    int tokenNum = sizeof(str2_r)/sizeof(char[20]);

    Scanner t(str);
    UBool pass = TRUE;
    t.getNextToken();
    int i = 0;
    while (pass){
        if (t.tokenType == STREAM_END){
            pass = pass? i == tokenNum : FALSE;
            break;//while
        } else if (t.tokenType == ERROR){
            pass = FALSE;
            break;//while
        } else {
            pass = strcmp( &(t.token[0]), str_r[i++]) == 0;
            t.getNextToken();
        }
    }

    //const char ts[] = "$commandList = '['"
    //" ( alternate ' ' $alternateOptions"
    //" | backwards ' 2'"
    //" | normalization ' ' $onoff "
    //" | caseLevel ' ' $onoff "
    //" | hiraganaQ ' ' $onoff"
    //" | caseFirst ' ' $caseFirstOptions"
    //" | strength ' ' $strengthOptions"
    //" ) ']';" ;

    //Scanner t2(ts);
    //pass = TRUE;
    //do {
    //    t2.getNextToken();
    //    if (t2.tokenType == ERROR){
    //        DumpScanner(t2);
    //        return FALSE;
    //    }
    //}while (t.tokenType != STREAM_END);

    return pass;
}

class TestParserT {
public:
UBool operator () (const char *const str, const int exp_error_offset = -1, const UBool dump = TRUE){
    Parser par(str);
    if (par.rules()){
        if ( exp_error_offset == -1){
            return TRUE;
        }else {
            DumpScanner(par.s,dump);
            return FALSE;
        }
    }else {
        return DumpScanner(par.s, dump) == exp_error_offset;
    }
}
};

UBool TestParser(){
    TestParserT test;

    UBool pass = TRUE;
    pass = pass && test ("$s = ' ' ? 50%;");
    pass = pass && test("$x = ($var {1,2}) 3%;");         // legal
    pass = pass && test("$x = $var {1,2} 3% | b 4%;");    // legal
    pass = pass && test("$x = $var {1,2} 3%;");           // legal
    pass = pass && test("$m = $c ? 2% 4% | $r 5% | $n 25%;"); // legal
    pass = pass && test("$a = b ? 2% | c 5%;");               // legal
    pass = pass && test("$x = A B 5% C 10% | D;", 8, FALSE);  // illegal 5%
    pass = pass && test("$x = aa 45% | bb 5% cc;", 19, FALSE);// illegal cc
    pass = pass && test("$x = (b 5%) (c 6%);");               // legal
    pass = pass && test("$x = (b 5%) c 6%;", 13, FALSE);      // illegal 6%
    pass = pass && test("$x = b 5% (c 6%);", 9, FALSE);       // illegal (c 6%)
    pass = pass && test("$x = b 5% c 6%;", 9, FALSE);         // illegal c 6%
    pass = pass && test("$x = b 5%;");                        // legal
    pass = pass && test("$x = aa 45% | bb 5% cc;", 19, FALSE);// illegal cc
    pass = pass && test("$x = a | b  | c 4% | d 5%;");        // legal
    pass = pass && test("$s = ' ' ? 50% abc;");               // legal
    pass = pass && test("$s =  a | c d | e f;");              // legal
    pass = pass && test( "$z = q 0% | p 1% | r 100%;");         // legal How to check parsed tree??

    pass = pass && test("$s = ' ' ? 50%;");
    pass = pass && test("$relationList = '<' | '<<' |  ';' | '<<<' | ',' | '=';");
    pass = pass && test("$p1 = ($string $s '|' $s)? 25%;");
    pass = pass && test("$p2 = (\\\\ $s $string $s)? 25%;");
    pass = pass && test("$rel2 = $p1 $string $s $p2;");
    pass = pass && test("$relation = $relationList $s ($rel1 | $rel2) $crlf;");
    pass = pass && test("$command = $commandList $crlf;");
    pass = pass && test("$reset = '&' $s ($beforeList $s)? 10% ($positionList 100% | $string 10%) $crlf;");
    pass = pass && test("$mostRules = $command 1% | $reset 5% | $relation 25%;");
    pass = pass && test("$root = $command{0,5} $reset $mostRules{1,20};");

    const char collationBNF[] =
    "$s = ' '? 50%;" 
    "$crlf = '\r\n';" 

    "$alternateOptions = non'-'ignorable | shifted;" 
    "$onoff = on | off;" 
    "$caseFirstOptions = off | upper | lower;" 
    "$strengthOptions = '1' | '2' | '3' | '4' | 'I';" 
    "$commandList = '['"
    " ( alternate ' ' $alternateOptions"
    " | backwards ' 2'"
    " | normalization ' ' $onoff "
    " | caseLevel ' ' $onoff "
    " | hiraganaQ ' ' $onoff"
    " | caseFirst ' ' $caseFirstOptions"
    " | strength ' ' $strengthOptions"
    " ) ']';" 
    "$command = $commandList $crlf;" 

    "$ignorableTypes = (tertiary | secondary | primary) ' ' ignorable;" 
    "$allTypes = variable | regular | implicit | trailing | $ignorableTypes;" 
    "$positionList = '[' (first | last) ' ' $allTypes ']';"

    "$beforeList = '[before ' ('1' | '2' | '3') ']';"

    "$relationList = ("
    "   '<'"
    " | '<<'"
    " | ';'" 
    " | '<<<'"
    " | ','" 
    " | '='"
    ");"
    "$string = $magic;" 
    "$rel1 = '[variable top]' $s;" 
    "$p1 = ($string $s '|' $s)? 25%;" 
    "$p2 = (\\\\ $s $string $s)? 25%;" 
    "$rel2 = $p1 $string $s $p2;" 
    "$relation = $relationList $s ($rel1 | $rel2) $crlf;" 

    "$reset = '&' $s ($beforeList $s)? 10% ($positionList 1% | $string 10%) $crlf;" 
    "$mostRules = $command 1% | $reset 5% | $relation 25%;"
    "$root = $command{0,5} $reset $mostRules{1,20};"
    ;
    
    pass = pass && test(collationBNF);


    return pass;
}

static UBool TestMorph(){
    srand((unsigned)time( NULL ));

    Alternation * alt = new Alternation();

    (*alt)
    .append(new Literal("a")).append(new Literal("b")).append(new Literal("c"))
    .append(new Literal("d")).append(new Literal("e")).append(new Literal("f"))
    .append(new Literal("g")).append(new Literal("h")).append(new Literal("i"))
    .append(new Literal("j")).append(new Literal("k")).append(new Literal("l"))
    .append(new Literal("m")).append(new Literal("n")).append(new Literal("o"))
    ;

    Repeat * rep = new Repeat( alt ,5,5 );
    Morph m( *rep);

//    DUMP_R(TestMorph,(*rep),20);
    DUMP_R(TestMorph,m,100);

    return FALSE;
}

#endif

static UBool TestLanguageGenerator(){
    //LanguageGenerator g;
    //const char *const s = "$s = p 0% | q 1%;";
    //g.parseBNF(s, "$s");
    UBool pass;
    //= strcmp("q", g.next()) == 0;

    const char *const def = 
        //"$a = $b;"
        //"$b = $c;"
        //"$c = $t;"
        //"$t = abc $z{1,2};"
        //"$k = a | b | c | d | e | f | g ;"
        //"$z = q 0% | p 1% | r 1%;"
        "$x = a ? 0%;"
        ; // end of string
//    const char * s = "abczz";
//
//
    LanguageGenerator g;
    pass = g.parseBNF(def, "$x",TRUE);
////    LanguageGenerator g(collationBNF, "$root", "$magic", new MagicNode());
//  
    if (pass != LanguageGenerator::OK) return FALSE;
    
    DUMP_R(TestLanguageGenerator, g, 20);
    return pass;

    ////UBool pass = strcmp(s,r) == 0;

    //if (pass){
    //    printf("TestRandomLanguageGenerator passed.\n");
    //} else {
    //    printf("TestRandomLanguageGenerator FAILED!!!\n");
    //}
    //return pass;
}

void TestWbnf(void){
    srand((unsigned)time( NULL ));

    //CALL(TestLiteral);
    //CALL(TestSequence);
    //CALL(TestSymbolTable);
    //CALL(TestVariable);

    //TestRepeat();
    //TestAlternation();
    //TestMorph();

    //TestQuote();
    //TestBuffer();
    //TestWeightedRand();

    //CALL(TestScanner);
    //CALL(TestParser);
    CALL(TestLanguageGenerator);
}

