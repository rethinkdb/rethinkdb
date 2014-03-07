/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_CALC8_VM_HPP)
#define BOOST_SPIRIT_CALC8_VM_HPP

#include <vector>

namespace client
{
    ///////////////////////////////////////////////////////////////////////////
    //  The Virtual Machine
    ///////////////////////////////////////////////////////////////////////////
    enum byte_code
    {
        op_neg,         //  negate the top stack entry
        op_add,         //  add top two stack entries
        op_sub,         //  subtract top two stack entries
        op_mul,         //  multiply top two stack entries
        op_div,         //  divide top two stack entries

        op_not,         //  boolean negate the top stack entry
        op_eq,          //  compare the top two stack entries for ==
        op_neq,         //  compare the top two stack entries for !=
        op_lt,          //  compare the top two stack entries for <
        op_lte,         //  compare the top two stack entries for <=
        op_gt,          //  compare the top two stack entries for >
        op_gte,         //  compare the top two stack entries for >=

        op_and,         //  logical and top two stack entries
        op_or,          //  logical or top two stack entries

        op_load,        //  load a variable
        op_store,       //  store a variable

        op_int,         //  push constant integer into the stack
        op_true,        //  push constant 0 into the stack
        op_false,       //  push constant 1 into the stack

        op_jump_if,     //  jump to a relative position in the code if top stack
                        //  evaluates to false
        op_jump,        //  jump to a relative position in the code

        op_stk_adj,     // adjust the stack (for args and locals)
        op_call,        // function call
        op_return       // return from function
    };

    class vmachine
    {
    public:

        vmachine(unsigned stackSize = 4096)
          : stack(stackSize)
        {
        }

        int execute(
            std::vector<int> const& code            // the program code
          , std::vector<int>::const_iterator pc     // program counter
          , std::vector<int>::iterator frame_ptr    // start of arguments and locals
        );

        int execute(std::vector<int> const& code)
        {
            return execute(code, code.begin(), stack.begin());
        };

        std::vector<int> stack;
    };
}

#endif

