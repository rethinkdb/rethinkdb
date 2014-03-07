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

// Tests the conversion of trigraph sequences.

// 1.1: The following 9 sequences are valid trigraph sequences.
//R #line 20 "t_5_001.cpp"
"??( ??) ??/??/ ??' ??< ??> ??! ??- ??="    //R "[ ] \\ ^ { } | ~ #" 
??( ??) ??/??/ ??' ??< ??> ??! ??- ??=      //R [ ] \\ ^ { } | ~ # 

// 1.2: In directive line.  
//R #line 26 "t_5_001.cpp"
??= define  OR(a, b)   a ??! b
OR(1, 2)                                    //R 1 | 2 

// 1.3: Any sequence other than above 9 is not a trigraph sequence.
//R #line 30 "t_5_001.cpp"
"?? ??? ??% ??^ ???="                       //R "?? ??? ??% ??^ ?#" 
?? ??? ??% ??^ ???=                         //R ? ? ? ? ? ? ?% ? ?^ ?# 

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

