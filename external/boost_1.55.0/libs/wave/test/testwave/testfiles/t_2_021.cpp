/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#define SIX (4 + 2.0)

//R #line 14 "t_2_021.cpp"
//R #pragma command option
#pragma command option
//R #line 17 "t_2_021.cpp"
//R #pragma command option (4 + 2.0)
#pragma command option SIX
//R #line 20 "t_2_021.cpp"
//R #pragma command option[(4 + 2.0)]
#pragma command option[SIX]
//R #line 23 "t_2_021.cpp"
//R #pragma command option(5)
#pragma command option(5)
//R #line 26 "t_2_021.cpp"
//R #pragma command option((4 + 2.0))
#pragma command option(SIX)
//R #line 29 "t_2_021.cpp"
//R #pragma command (4 + 2.0)
#pragma command SIX

//H 10: t_2_021.cpp(10): #define
//H 08: t_2_021.cpp(10): SIX=(4 + 2.0)
//H 10: t_2_021.cpp(14): #pragma
//H 10: t_2_021.cpp(17): #pragma
//H 01: t_2_021.cpp(10): SIX
//H 02: (4 + 2.0)
//H 03: (4 + 2.0)
//H 10: t_2_021.cpp(20): #pragma
//H 01: t_2_021.cpp(10): SIX
//H 02: (4 + 2.0)
//H 03: (4 + 2.0)
//H 10: t_2_021.cpp(23): #pragma
//H 10: t_2_021.cpp(26): #pragma
//H 01: t_2_021.cpp(10): SIX
//H 02: (4 + 2.0)
//H 03: (4 + 2.0)
//H 10: t_2_021.cpp(29): #pragma
//H 01: t_2_021.cpp(10): SIX
//H 02: (4 + 2.0)
//H 03: (4 + 2.0)
