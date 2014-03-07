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

// Tests grouping of sub-expressions in #if expression.

// 13.8: Unary operators are grouped from right to left.
//R #line 24 "t_5_018.cpp"
//R true
#if (- -1 != 1) || (!!9 != 1) || (-!+!9 != -1) || (~~1 != 1)
    "Bad grouping of -, +, !, ~ in #if expression."
#else
true
#endif

// 13.9: ?: operators are grouped from right to left.
//R #line 33 "t_5_018.cpp"
//R true
#if (1 ? 2 ? 3 ? 3 : 2 : 1 : 0) != 3
    "Bad grouping of ? : in #if expression."
#else
true
#endif

// 13.10: Other operators are grouped from left to right.
//R #line 42 "t_5_018.cpp"
//R true
#if (15 >> 2 >> 1 != 1) || (3 << 2 << 1 != 24)
    "Bad grouping of >>, << in #if expression."
#else
true
#endif

// 13.11: Test of precedence.
//R #line 51 "t_5_018.cpp"
//R true
#if 3*10/2 >> !0*2 >> !+!-9 != 1
    "Bad grouping of -, +, !, *, /, >> in #if expression."
#else
true
#endif

// 13.12: Overall test.  Grouped as:
//        ((((((+1 - -1 - ~~1 - -!0) & 6) | ((8 % 9) ^ (-2 * -2))) >> 1) == 7)
//        ? 7 : 0) != 7
//    evaluates to false.
//R #line 63 "t_5_018.cpp"
//R true
#if (((+1- -1-~~1- -!0&6|8%9^-2*-2)>>1)==7?7:0)!=7
    "Bad arithmetic of #if expression."
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

