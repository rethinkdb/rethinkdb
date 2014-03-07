// Copyright (C) 2008 Jurko Gospodnetic
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


//   This tests whether the Boost Pool library managed to get a regression and
// hit the MSVC 'variables exported to global namespace' bug again. This bug
// affects at least MSVC 7.1 & 8.0 releases and has been fixed in the MSVC 9.0
// release.
//
//   If the bug exists this test should fail to compile, complaining about an
// ambiguous CRITICAL_SECTION symbol. The bug got fixed by making the boost/
// /pool/detail/mutex.hpp header reference all Windows API constants using their
// fully qualified names.
//
//   To see the bug in action without using any Boost libraries run the
// following program:
//
//     namespace One { class Brick; }
//     namespace Two
//     {
//         using namespace One;
//         template <class TinyTemplateParam> class TinyClass {};
//     }
//     class Brick {};
//     Brick brick;
//     int main() {}
//                                                   (17.04.2008.) (Jurko)


#include "boost/archive/text_iarchive.hpp"
#include "boost/pool/detail/mutex.hpp"
//   Including "boost/pool/pool_alloc.hpp" instead of mutex.hpp should work as
// well.

int main()
{
}
