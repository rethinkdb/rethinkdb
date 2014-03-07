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

// Tests valid operators in #if expression.

//  Valid operators are (precedence in this order) :
//    defined, (unary)+, (unary)-, ~, !,
//    *, /, %,
//    +, -,
//    <<, >>,
//    <, >, <=, >=,
//    ==, !=,
//    &,
//    ^,
//    |,
//    &&,
//    ||,
//    ? :

// 13.1: Bit shift.
//R #line 38 "t_5_015.cpp"
//R true
#if 1 << 2 != 4 || 8 >> 1 != 4
    "Bad arithmetic of <<, >> operators."
#else
true
#endif

// 13.2: Bitwise operators.
//R #line 47 "t_5_015.cpp"
//R true
#if (3 ^ 5) != 6 || (3 | 5) != 7 || (3 & 5) != 1
    "Bad arithmetic of ^, |, & operators."
#else
true
#endif

// 13.3: Result of ||, && operators is either of 1 or 0.
//R #line 56 "t_5_015.cpp"
//R true
#if (2 || 3) != 1 || (2 && 3) != 1 || (0 || 4) != 1 || (0 && 5) != 0
    "Bad arithmetic of ||, && operators."
#else
true
#endif

// 13.4: ?, : operator.
//R #line 65 "t_5_015.cpp"
//R true
#if (0 ? 1 : 2) != 2
    "Bad arithmetic of ?: operator.";
#else
true
#endif

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

