/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_CALC7_VM_HPP)
#define BOOST_SPIRIT_CALC7_VM_HPP

#include <vector>

namespace client
{
    ///////////////////////////////////////////////////////////////////////////
    //  The Virtual Machine
    ///////////////////////////////////////////////////////////////////////////
    enum byte_code
    {
        op_neg,     //  negate the top stack entry
        op_add,     //  add top two stack entries
        op_sub,     //  subtract top two stack entries
        op_mul,     //  multiply top two stack entries
        op_div,     //  divide top two stack entries

        op_load,    //  load a variable
        op_store,   //  store a variable
        op_int,     //  push constant integer into the stack
        op_stk_adj  //  adjust the stack for local variables
    };

    class vmachine
    {
    public:

        vmachine(unsigned stackSize = 4096)
          : stack(stackSize)
          , stack_ptr(stack.begin())
        {
        }

        void execute(std::vector<int> const& code);
        std::vector<int> const& get_stack() const { return stack; };

    private:

        std::vector<int> stack;
        std::vector<int>::iterator stack_ptr;
    };
}

#endif

