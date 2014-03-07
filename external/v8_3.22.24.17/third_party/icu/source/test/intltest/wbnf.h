/*
 ******************************************************************************
 * Copyright (C) 2005, International Business Machines Corporation and   *
 * others. All Rights Reserved.                                               *
 ******************************************************************************
 */
/*
  WBNF, Weighted BNF, is an extend BNF. The most difference between WBNF 
  and standard BNF is the WBNF accepts weight for its alternation items.
  The weight specifies the opportunity it will be selected.

  The purpose of WBNF is to help generate a random string from a given grammar
  which can be described with standard BNF. The introduction of 'weight'
  is to guide the generator to give the specific parts different chances to be 
  generated.

  Usually, the user gives LanguageGenerator the grammar description in WBNF,
  then LanguageGenerator will generate a random string on every next() call.
  The return code of parseBNF() can help user to determine the error,
  either in the grammar description or in the WBNF parser itself.


  The grammar of WBNF itself can be described in standard BNF, 
  
    escaping        = _single character with a leading back slash, either inside or outside quoting_
    quoting         = _quoted with a pair of single quotation marks_
    string          = string alphabet | string digit | string quoting | string escaping |
                      alphabet | quoting | escaping
    alphabet        = 
    digit           = 
    integer         = integer digit | digit
    weight          = integer %
    weight-list     = weight-list weight | weight
    var             = var alphabet | var digit | $ alphabet

    var-defs        = var-defs var-def | var-def
    var-def         = var '=' definition;

    alternation     = alternation '|' alt-item | alt-item 
    alt-item        = sequence | sequence weight

    sequence        = sequence modified | modified

    modified        = core | morph | quote | repeat
    morph           = modified ~
    quote           = modified @
    repeat          = modified quantifier | modified quantifier weight-list
    quantifier      = ? | * | + | { integer , integer} | {integer, } | {integer}

    core            = var | string | '(' definition ')'

    definition      = core | modified | sequence | alternation
    definition      = alternation

    Remarks:
    o Following characters are literals in preceding definition
      but are syntax symbols in WBNF

      % $ ~ @ ? * + { } ,

    o Following character are syntax symbols in preceding definition
              (sapce) contact operation, or separators to increase readability
      =       definition
      |       selection operation
      ( )     precedence select
      ' '     override special-character to plain character

    o the definition of 'escaping' and 'quoting' are preceding definition text
    o infinite is actually a predefine value PSEUDO_INFINIT defined in this file 
    o if weight is not presented in "alt-item' and 'repeat',
      a default weight DEFAULT_WEIGHT defined in this file is used

    o * == {0,  }
      + == {1,  }
      ? == {0, 1}

    o the weight-list for repeat assigns the weights for repeat itmes one by one
      
      demo{1,3} 30% 40% 100%  ==  (demo)30% | (demodemo)40% | (demodemodemo)100%

      To find more explain of the weight-list, please see the LIMITATION of the grammar

    o but the weight-list for question mark has different meaning

      demo ? 30%   != demo{0,1} 30% 100%
      demo ? 30%   == demo{0,1} 70% 30%
      
      the 70% is calculated from (DEFAULT_WEIGHT - weight)


  Known LIMITATION of the grammar
    For 'repeat', the parser will eat up as much as possible weights at one time,
    discard superfluous weights if it is too much,
    fill insufficient weights with default weight if it is too less.
    This behavior means following definitions are equal

        demo{1,3} 30% 40% 100%
        demo{1,3} 30% 40% 100% 50%
        demo{1,3} 30% 40%

    This behavior will cause a little confusion when defining an alternation

        demo{1,3} 30% 40% 100% 50% | show 20%

    is interpreted as 

        (demo{1,3} 30% 40% 100%) 100% | show 20%

    not 

        (demo{1,3} 30% 40% 100%) 50% | show 20%

    to get an expected definition, please use parentheses.

  Known LIMITATION of current implement
    Due to the well known point alias problem, current Parser will be effectively
    crashed if the definition looks like

        $a = demo;
        $b = $a;
        $c = $a;
    or
        $a = demo;
        $b = $a $a;
    or 
        $a = demo;
        $b = $b $a;

    The crash will occur at delete operation in destructor or other memory release code.
    Several plans are on hard to fix the problem. Use a smart point with reference count,
    or use a central memory management solution. But now, it works well with collation 
    monkey test, which is the only user for WBNF.
*/

#ifndef _WBNF
#define _WBNF

#include "unicode/utypes.h"

const int DEFAULT_WEIGHT = 100;
const int PSEUDO_INFINIT = 200;

class LanguageGenerator_impl;

class LanguageGenerator{
    LanguageGenerator_impl * lang_gen;
public:
    enum PARSE_RESULT {OK, BNF_DEF_WRONG, INCOMPLETE, NO_TOP_NODE};
    LanguageGenerator();
    ~LanguageGenerator();
    PARSE_RESULT parseBNF(const char *const bnf_definition /*in*/, const char *const top_node/*in*/, UBool debug=FALSE);
    const char *next(); /* Return a null-terminated c-string. The buffer is owned by callee. */
};

void TestWbnf(void);

#endif /* _WBNF */
