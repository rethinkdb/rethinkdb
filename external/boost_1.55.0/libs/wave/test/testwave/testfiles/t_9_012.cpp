/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests, whether macro definition works when there is no whitespace in between
// the macro name and the defined expansion list

#define CreateWindowA(lpClassName, lpWindowName, dwStyle, x, y,\
nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam)\
CreateWindowExA(0L, lpClassName, lpWindowName, dwStyle, x, y,\
nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam)

//R #line 19 "t_9_012.cpp"
CreateWindowA(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11)  //R CreateWindowExA(0L, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11) 

//H 10: t_9_012.cpp(13): #define
//H 08: t_9_012.cpp(13): CreateWindowA(lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam)=CreateWindowExA(0L, lpClassName, lpWindowName, dwStyle, x, y,nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam)
//H 00: t_9_012.cpp(19): CreateWindowA(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11), [t_9_012.cpp(13): CreateWindowA(lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam)=CreateWindowExA(0L, lpClassName, lpWindowName, dwStyle, x, y,nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam)]
//H 02: CreateWindowExA(0L, 1,  2,  3,  4,  5, 6,  7,  8,  9,  10,  11)
//H 03: CreateWindowExA(0L, 1,  2,  3,  4,  5, 6,  7,  8,  9,  10,  11)
