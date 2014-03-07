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

// Tests the line splicing by <backslash><newline> sequence.

// 2.1: In a #define directive line, between the parameter list and the
//      replacement text.   
//R #line 23 "t_5_002.cpp"
#define FUNC(a, b, c) \
        a ## b ## c
FUNC(ab, cd, ef)        //R abcdef 

// 2.2: In a #define directive line, among the parameter list and among the
//      replacement text.
//R #line 33 "t_5_002.cpp"
#undef  FUNC
#define FUNC(a, b     \
    , c)              \
    a ## b            \
    ## c
FUNC(ab, cd, ef)        //R abcdef 

// 2.3: In a string literal.
//R #line 37 "t_5_002.cpp"
"abc\
de"                     //R "abcde" 

// 2.4: <backslash><newline> in midst of an identifier.
//R #line 43 "t_5_002.cpp"
#define ABCDE 5
ABC\
DE                      //R 5 

// 2.5: <backslash><newline> by trigraph. 
//R #line 48 "t_5_002.cpp"
ABC??/
DE                      //R 5 

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

