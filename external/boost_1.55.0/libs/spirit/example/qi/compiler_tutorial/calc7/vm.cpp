/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include "vm.hpp"

namespace client
{
    void vmachine::execute(std::vector<int> const& code)
    {
        std::vector<int>::const_iterator pc = code.begin();
        std::vector<int>::iterator locals = stack.begin();
        stack_ptr = stack.begin();

        while (pc != code.end())
        {
            switch (*pc++)
            {
                case op_neg:
                    stack_ptr[-1] = -stack_ptr[-1];
                    break;

                case op_add:
                    --stack_ptr;
                    stack_ptr[-1] += stack_ptr[0];
                    break;

                case op_sub:
                    --stack_ptr;
                    stack_ptr[-1] -= stack_ptr[0];
                    break;

                case op_mul:
                    --stack_ptr;
                    stack_ptr[-1] *= stack_ptr[0];
                    break;

                case op_div:
                    --stack_ptr;
                    stack_ptr[-1] /= stack_ptr[0];
                    break;

                case op_load:
                    *stack_ptr++ = locals[*pc++];
                    break;

                case op_store:
                    --stack_ptr;
                    locals[*pc++] = stack_ptr[0];
                    break;

                case op_int:
                    *stack_ptr++ = *pc++;
                    break;

                case op_stk_adj:
                    stack_ptr = stack.begin() + *pc++;
                    break;
            }
        }
    }
}


