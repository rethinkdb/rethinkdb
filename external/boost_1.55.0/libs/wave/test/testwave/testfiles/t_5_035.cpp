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

// Tests translation limits.

#define ABCDEFGHIJKLMNOPQRSTUVWXYZabcde 0
#define ABCDEFGHIJKLMNOPQRSTUVWXYZabcd_ 1

// 37.1: Number of parameters in macro: at least 31.
#define GLUE31(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E)   \
    a##b##c##d##e##f##g##h##i##j##k##l##m##n##o##p##q##r##s##t##u##v##w##x##y##z##A##B##C##D##E

// 37.2: Number of arguments in macro call: at least 31.
//R #line 28 "t_5_035.cpp"
//R 0
GLUE31( A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R,
    S, T, U, V, W, X, Y, Z, a, b, c, d, e)

// 37.3: Significant initial characters in an internal identifier or
//       macro name: at least 31.
//R #line 34 "t_5_035.cpp"
ABCDEFGHIJKLMNOPQRSTUVWXYZabcd_         //R 1 

// 37.4: Nested conditional inclusion: at least 8 levels.
//R #line 65 "t_5_035.cpp"
#define NEST 0
#ifdef  A
#else
#   ifdef   B
#   else
#       ifdef   C
#       else
#           ifdef   D
#           else
#               ifdef   E
#               else
#                   ifdef   F
#                   else
#                       ifdef   G
#                       else
#                           ifdef   H
#                           else
#undef NEST
#define NEST 1
#                           endif
#                       endif
#                   endif
#               endif
#           endif
#       endif
#   endif
#endif
NEST                                  //R 1 

// 37.5: Nested source file inclusion: at least 8 levels.
//R #line 70 "t_5_035.cpp"
#include "t_5_035_01.hpp"
NEST                                  //R 8 

// 37.6: Parenthesized expression: at least 32 levels.
//R #line 81 "t_5_035.cpp"
#if  0 + (1 - (2 + (3 - (4 + (5 - (6 + (7 - (8 + (9 - (10 + (11 - (12 +  \
     (13 - (14 + (15 - (16 + (17 - (18 + (19 - (20 + (21 - (22 + (23 -   \
     (24 + (25 - (26 + (27 - (28 + (29 - (30 + (31 - (32 + 0))))))))))   \
     )))))))))))))))))))))) == 0
#undef NEST
#define NEST 32
#endif
NEST                                  //R 32 

// 37.7: Characters in a string (after concatenation): at least 509.
//R #line 86 "t_5_035.cpp"
//R "123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567"
"123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789\
012345678901234567"

// 37.8: Characters in a logical source line: at least 509.
//R #line 98 "t_5_035.cpp"
//R 123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567
123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789\
012345678901234567

// 37.9: Macro definitions: at least 1024.
//R #line 110 "t_5_035.cpp"
#include "t_5_035.hpp"
ZX                                    //R 1 

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

