/*=============================================================================
    Copyright (c) 2003 Jonathan de Halleux (dehalleux@pelikhan.com)
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include "action_tests.hpp"

int
main()
{
    assign_action_test();
    assign_key_action_test();
    clear_action_test();
    decrement_action_test();
    erase_action_test();
    increment_action_test();
    insert_key_action_test();
    push_front_action_test();
    push_back_action_test();
    swap_action_test();

    return boost::report_errors();
}
