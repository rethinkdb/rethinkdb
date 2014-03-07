/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    The tests included in this file were initially taken from the mcpp V2.5
    preprocessor validation suite and were modified to fit into the Boost.Wave 
    unit test requirements.
    The original files of the mcpp preprocessor are distributed under the 
    license reproduced at the end of this file.
=============================================================================*/

// Tests whether macro arguments are pre-expanded (unless the argument is an
// operand of # or ## operator) separately, that is, are macro-replaced
// completely prior to rescanning.

#define ZERO_TOKEN
#define MACRO_0         0
#define MACRO_1         1
#define TWO_ARGS        a,b
#define SUB(x, y)       (x - y)
#define GLUE(a, b)      a ## b
#define XGLUE(a, b)     GLUE( a, b)
#define STR(a)          # a

// 25.1: "TWO_ARGS" is read as one argument to "SUB", then expanded to
//       "a,b", then "x" is substituted by "a,b".
//R #line 32 "t_5_028.cpp"
SUB(TWO_ARGS, 1)              //R (a,b - 1) 

// 25.2: An argument pre-expanded to 0-token.    */
//R #line 36 "t_5_028.cpp"
SUB(ZERO_TOKEN, a)            //R ( - a) 

// 25.3: "glue( a, b)" is pre-expanded.  */
//R #line 40 "t_5_028.cpp"
XGLUE(GLUE(a, b), c)          //R abc 

// 25.4: Operands of ## operator are not pre-expanded.
//R #line 44 "t_5_028.cpp"
GLUE(MACRO_0, MACRO_1)        //R MACRO_0MACRO_1 

// 25.5: Operand of # operator is not pre-expanded.
//R #line 48 "t_5_028.cpp"
STR(ZERO_TOKEN)               //R "ZERO_TOKEN" 

/*-
 * Copyright (c) 1998, 2002-2005 Kiyoshi Matsui <kmatsui@t3.rim.or.jp>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

