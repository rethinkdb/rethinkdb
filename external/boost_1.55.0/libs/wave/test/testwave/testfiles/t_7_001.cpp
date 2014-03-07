/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

//O --c++11

//R #line 16 "t_7_001.cpp"
//R R"de
//R fg
//R h"
R"de
fg
h"

//R #line 21 "t_7_001.cpp"
"abc"   //R "abc" 
R"abc"  //R R"abc" 

//R #line 27 "t_7_001.cpp"
//R uR"de fg
//R h"
uR"de \
fg
h"

//R #line 32 "t_7_001.cpp"
u"abc"      //R u"abc" 
U"def"      //R U"def" 
u8"ghi"     //R u8"ghi" 
