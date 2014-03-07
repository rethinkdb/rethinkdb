/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// This sample is taken from the C++ standard 16.3.5.5 [cpp.scope]

// Currently this test fails due to a pathologic border case, which is included
// here. Wave currently does not support expanding macros, where the 
// replacement-list terminates in partial macro expansion (such as the 
// definition of the macro h below). This will be fixed in a future release.

#define x 3
#define f(a) f(x * (a))
#undef x
#define x 2
#define g f
#define z z[0]
#define h g( ~
#define m(a) a(w)
#define w 0,1
#define t(a) a
f(y+1) + f(f(z)) % t(t(g)(0) + t)(1);
g(x+(3,4)-w)
h 5) & m(f)^m(m);

//R #line 27 "t_1_014.cpp"
//R f(2 * (y+1)) + f(2 * (f(2 * (z[0])))) % f(2 * (0)) + t(1);
//R f(2 * (2+(3,4)-0,1))
//E t_1_014.cpp(29): error: improperly terminated macro invocation or replacement-list terminates in partial macro expansion (not supported yet): missing ')'
// should expand to: f(2 * g( ~ 5)) & f(2 * (0,1))^m(0,1);
