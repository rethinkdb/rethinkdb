/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_CONJURE_IDS_HPP)
#define BOOST_SPIRIT_CONJURE_IDS_HPP

namespace client
{
    struct op_type
    {
        enum type
        {
            binary = 0x20000,
            unary = 0x40000,
            assign = 0x80000
        };
    };

    struct op
    {
        enum type
        {
            // binary
            comma,
            assign,
            plus_assign,
            minus_assign,
            times_assign,
            divide_assign,
            mod_assign,
            bit_and_assign,
            bit_xor_assign,
            bit_or_assign,
            shift_left_assign,
            shift_right_assign,
            logical_or,
            logical_and,
            bit_or,
            bit_xor,
            bit_and,
            equal,
            not_equal,
            less,
            less_equal,
            greater,
            greater_equal,
            shift_left,
            shift_right,
            plus,
            minus,
            times,
            divide,
            mod,

            // unary
            plus_plus,
            minus_minus,
            compl_,
            not_,
        };
    };

    template <int type, int op>
    struct make_op
    {
        static int const value = type + op;
    };

    template <op::type op>
    struct unary_op : make_op<op_type::unary, op> {};

    template <op::type op>
    struct binary_op
        : make_op<op_type::binary, op> {};

    template <op::type op>
    struct assign_op
        : make_op<op_type::assign, op> {};

    template <op::type op>
    struct binary_or_unary_op
        : make_op<op_type::unary | op_type::binary, op> {};

    struct token_ids
    {
        enum type
        {
            // pseudo tags
            invalid             = -1,
            op_binary           = op_type::binary,
            op_unary            = op_type::unary,
            op_assign           = op_type::assign,

            // binary / unary operators with common tokens
            // '+' and '-' can be binary or unary
            // (the lexer cannot distinguish which)
            plus                = binary_or_unary_op<op::plus>::value,
            minus               = binary_or_unary_op<op::minus>::value,

            // binary operators
            comma               = binary_op<op::comma>::value,
            assign              = assign_op<op::assign>::value,
            plus_assign         = assign_op<op::plus_assign>::value,
            minus_assign        = assign_op<op::minus_assign>::value,
            times_assign        = assign_op<op::times_assign>::value,
            divide_assign       = assign_op<op::divide_assign>::value,
            mod_assign          = assign_op<op::mod_assign>::value,
            bit_and_assign      = assign_op<op::bit_and_assign>::value,
            bit_xor_assign      = assign_op<op::bit_xor_assign>::value,
            bit_or_assign       = assign_op<op::bit_or_assign>::value,
            shift_left_assign   = assign_op<op::shift_left_assign>::value,
            shift_right_assign  = assign_op<op::shift_right_assign>::value,
            logical_or          = binary_op<op::logical_or>::value,
            logical_and         = binary_op<op::logical_and>::value,
            bit_or              = binary_op<op::bit_or>::value,
            bit_xor             = binary_op<op::bit_xor>::value,
            bit_and             = binary_op<op::bit_and>::value,
            equal               = binary_op<op::equal>::value,
            not_equal           = binary_op<op::not_equal>::value,
            less                = binary_op<op::less>::value,
            less_equal          = binary_op<op::less_equal>::value,
            greater             = binary_op<op::greater>::value,
            greater_equal       = binary_op<op::greater_equal>::value,
            shift_left          = binary_op<op::shift_left>::value,
            shift_right         = binary_op<op::shift_right>::value,
            times               = binary_op<op::times>::value,
            divide              = binary_op<op::divide>::value,
            mod                 = binary_op<op::mod>::value,

            // unary operators with overlaps
            // '++' and '--' can be prefix or postfix
            // (the lexer cannot distinguish which)
            plus_plus           = unary_op<op::plus_plus>::value,
            minus_minus         = unary_op<op::minus_minus>::value,

            // unary operators
            compl_              = unary_op<op::compl_>::value,
            not_                = unary_op<op::not_>::value,

            // misc tags
            identifier          = op::not_ + 1,
            comment,
            whitespace,
            lit_uint,
            true_or_false
        };
    };
}

#endif
