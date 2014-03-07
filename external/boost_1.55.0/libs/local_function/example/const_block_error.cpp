
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#include "const_block.hpp"
#include <cassert>

int main(void) {
    //[const_block
    int x = 1, y = 2;
    CONST_BLOCK(x, y) { // Constant block.
        assert(x = y);  // Compiler error.
    } CONST_BLOCK_END
    //]
    return 0;
}

