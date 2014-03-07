/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include "config.hpp"
#include "compiler.hpp"
#include "vm.hpp"
#include <boost/foreach.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/assert.hpp>
#include <boost/lexical_cast.hpp>
#include <set>

namespace client { namespace code_gen
{
    void function::op(int a)
    {
        code.push_back(a);
        size_ += 1;
    }

    void function::op(int a, int b)
    {
        code.push_back(a);
        code.push_back(b);
        size_ += 2;
    }

    void function::op(int a, int b, int c)
    {
        code.push_back(a);
        code.push_back(b);
        code.push_back(c);
        size_ += 3;
    }

    int const* function::find_var(std::string const& name) const
    {
        std::map<std::string, int>::const_iterator i = variables.find(name);
        if (i == variables.end())
            return 0;
        return &i->second;
    }

    void function::add_var(std::string const& name)
    {
        std::size_t n = variables.size();
        variables[name] = n;
    }

    void function::link_to(std::string const& name, std::size_t address)
    {
        function_calls[address] = name;
    }

    void function::print_assembler() const
    {
        std::vector<int>::const_iterator pc = code.begin() + address;

        std::vector<std::string> locals(variables.size());
        typedef std::pair<std::string, int> pair;
        BOOST_FOREACH(pair const& p, variables)
        {
            locals[p.second] = p.first;
            std::cout << "      local       "
                << p.first << ", @" << p.second << std::endl;
        }

        std::map<std::size_t, std::string> lines;
        std::set<std::size_t> jumps;

        while (pc != (code.begin() + address + size_))
        {
            std::string line;
            std::size_t address = pc - code.begin();

            switch (*pc++)
            {
                case op_neg:
                    line += "      op_neg";
                    break;

                case op_not:
                    line += "      op_not";
                    break;

                case op_add:
                    line += "      op_add";
                    break;

                case op_sub:
                    line += "      op_sub";
                    break;

                case op_mul:
                    line += "      op_mul";
                    break;

                case op_div:
                    line += "      op_div";
                    break;

                case op_eq:
                    line += "      op_eq";
                    break;

                case op_neq:
                    line += "      op_neq";
                    break;

                case op_lt:
                    line += "      op_lt";
                    break;

                case op_lte:
                    line += "      op_lte";
                    break;

                case op_gt:
                    line += "      op_gt";
                    break;

                case op_gte:
                    line += "      op_gte";
                    break;

                case op_and:
                    line += "      op_and";
                    break;

                case op_or:
                    line += "      op_or";
                    break;

                case op_load:
                    line += "      op_load     ";
                    line += boost::lexical_cast<std::string>(locals[*pc++]);
                    break;

                case op_store:
                    line += "      op_store    ";
                    line += boost::lexical_cast<std::string>(locals[*pc++]);
                    break;

                case op_int:
                    line += "      op_int      ";
                    line += boost::lexical_cast<std::string>(*pc++);
                    break;

                case op_true:
                    line += "      op_true";
                    break;

                case op_false:
                    line += "      op_false";
                    break;

                case op_jump:
                    {
                        line += "      op_jump     ";
                        std::size_t pos = (pc - code.begin()) + *pc++;
                        if (pos == code.size())
                            line += "end";
                        else
                            line += boost::lexical_cast<std::string>(pos);
                        jumps.insert(pos);
                    }
                    break;

                case op_jump_if:
                    {
                        line += "      op_jump_if  ";
                        std::size_t pos = (pc - code.begin()) + *pc++;
                        if (pos == code.size())
                            line += "end";
                        else
                            line += boost::lexical_cast<std::string>(pos);
                        jumps.insert(pos);
                    }
                    break;

                case op_call:
                    {
                        line += "      op_call     ";
                        int nargs = *pc++;
                        std::size_t jump = *pc++;
                        line += boost::lexical_cast<std::string>(nargs) + ", ";
                        BOOST_ASSERT(function_calls.find(jump) != function_calls.end());
                        line += function_calls.find(jump)->second;
                    }
                    break;

                case op_stk_adj:
                    line += "      op_stk_adj  ";
                    line += boost::lexical_cast<std::string>(*pc++);
                    break;


                case op_return:
                    line += "      op_return";
                    break;
            }
            lines[address] = line;
        }

        std::cout << "start:" << std::endl;
        typedef std::pair<std::size_t, std::string> line_info;
        BOOST_FOREACH(line_info const& l, lines)
        {
            std::size_t pos = l.first;
            if (jumps.find(pos) != jumps.end())
                std::cout << pos << ':' << std::endl;
            std::cout << l.second << std::endl;
        }

        std::cout << "end:" << std::endl << std::endl;
    }

    bool compiler::operator()(unsigned int x)
    {
        BOOST_ASSERT(current != 0);
        current->op(op_int, x);
        return true;
    }

    bool compiler::operator()(bool x)
    {
        BOOST_ASSERT(current != 0);
        current->op(x ? op_true : op_false);
        return true;
    }

    bool compiler::operator()(ast::identifier const& x)
    {
        BOOST_ASSERT(current != 0);
        int const* p = current->find_var(x.name);
        if (p == 0)
        {
            error_handler(x.id, "Undeclared variable: " + x.name);
            return false;
        }
        current->op(op_load, *p);
        return true;
    }

    bool compiler::operator()(token_ids::type const& x)
    {
        BOOST_ASSERT(current != 0);
        switch (x)
        {
            case token_ids::plus: current->op(op_add); break;
            case token_ids::minus: current->op(op_sub); break;
            case token_ids::times: current->op(op_mul); break;
            case token_ids::divide: current->op(op_div); break;

            case token_ids::equal: current->op(op_eq); break;
            case token_ids::not_equal: current->op(op_neq); break;
            case token_ids::less: current->op(op_lt); break;
            case token_ids::less_equal: current->op(op_lte); break;
            case token_ids::greater: current->op(op_gt); break;
            case token_ids::greater_equal: current->op(op_gte); break;

            case token_ids::logical_or: current->op(op_or); break;
            case token_ids::logical_and: current->op(op_and); break;
            default: BOOST_ASSERT(0); return false;
        }
        return true;
    }

    bool compiler::operator()(ast::unary const& x)
    {
        BOOST_ASSERT(current != 0);
        if (!boost::apply_visitor(*this, x.operand_))
            return false;
        switch (x.operator_)
        {
            case token_ids::minus: current->op(op_neg); break;
            case token_ids::not_: current->op(op_not); break;
            case token_ids::plus: break;
            default: BOOST_ASSERT(0); return false;
        }
        return true;
    }

    bool compiler::operator()(ast::function_call const& x)
    {
        BOOST_ASSERT(current != 0);

        if (functions.find(x.function_name.name) == functions.end())
        {
            error_handler(x.function_name.id, "Function not found: " + x.function_name.name);
            return false;
        }

        boost::shared_ptr<code_gen::function> p = functions[x.function_name.name];

        if (p->nargs() != x.args.size())
        {
            error_handler(x.function_name.id, "Wrong number of arguments: " + x.function_name.name);
            return false;
        }

        BOOST_FOREACH(ast::expression const& expr, x.args)
        {
            if (!(*this)(expr))
                return false;
        }

        current->op(
            op_call,
            p->nargs(),
            p->get_address());
        current->link_to(x.function_name.name, p->get_address());

        return true;
    }

    namespace
    {
        int precedence[] = {
            // precedence 1
            1, // op_comma

            // precedence 2
            2, // op_assign
            2, // op_plus_assign
            2, // op_minus_assign
            2, // op_times_assign
            2, // op_divide_assign
            2, // op_mod_assign
            2, // op_bit_and_assign
            2, // op_bit_xor_assign
            2, // op_bitor_assign
            2, // op_shift_left_assign
            2, // op_shift_right_assign

            // precedence 3
            3, // op_logical_or

            // precedence 4
            4, // op_logical_and

            // precedence 5
            5, // op_bit_or

            // precedence 6
            6, // op_bit_xor

            // precedence 7
            7, // op_bit_and

            // precedence 8
            8, // op_equal
            8, // op_not_equal

            // precedence 9
            9, // op_less
            9, // op_less_equal
            9, // op_greater
            9, // op_greater_equal

            // precedence 10
            10, // op_shift_left
            10, // op_shift_right

            // precedence 11
            11, // op_plus
            11, // op_minus

            // precedence 12
            12, // op_times
            12, // op_divide
            12  // op_mod
        };
    }

    inline int precedence_of(token_ids::type op)
    {
        return precedence[op & 0xFF];
    }

    // The Shunting-yard algorithm
    bool compiler::compile_expression(
        int min_precedence,
        std::list<ast::operation>::const_iterator& rbegin,
        std::list<ast::operation>::const_iterator rend)
    {
        while ((rbegin != rend) && (precedence_of(rbegin->operator_) >= min_precedence))
        {
            token_ids::type op = rbegin->operator_;
            if (!boost::apply_visitor(*this, rbegin->operand_))
                return false;
            ++rbegin;

            while ((rbegin != rend) && (precedence_of(rbegin->operator_) > precedence_of(op)))
            {
                token_ids::type next_op = rbegin->operator_;
                compile_expression(precedence_of(next_op), rbegin, rend);
            }
            (*this)(op);
        }
        return true;
    }

    bool compiler::operator()(ast::expression const& x)
    {
        BOOST_ASSERT(current != 0);
        if (!boost::apply_visitor(*this, x.first))
            return false;
        std::list<ast::operation>::const_iterator rbegin = x.rest.begin();
        if (!compile_expression(0, rbegin, x.rest.end()))
            return false;
        return true;
    }

    bool compiler::operator()(ast::assignment const& x)
    {
        BOOST_ASSERT(current != 0);
        if (!(*this)(x.rhs))
            return false;
        int const* p = current->find_var(x.lhs.name);
        if (p == 0)
        {
            error_handler(x.lhs.id, "Undeclared variable: " + x.lhs.name);
            return false;
        }
        current->op(op_store, *p);
        return true;
    }

    bool compiler::operator()(ast::variable_declaration const& x)
    {
        BOOST_ASSERT(current != 0);
        int const* p = current->find_var(x.lhs.name);
        if (p != 0)
        {
            error_handler(x.lhs.id, "Duplicate variable: " + x.lhs.name);
            return false;
        }
        if (x.rhs) // if there's an RHS initializer
        {
            bool r = (*this)(*x.rhs);
            if (r) // don't add the variable if the RHS fails
            {
                current->add_var(x.lhs.name);
                current->op(op_store, *current->find_var(x.lhs.name));
            }
            return r;
        }
        else
        {
            current->add_var(x.lhs.name);
        }
        return true;
    }

    bool compiler::operator()(ast::statement const& x)
    {
        BOOST_ASSERT(current != 0);
        return boost::apply_visitor(*this, x);
    }

    bool compiler::operator()(ast::statement_list const& x)
    {
        BOOST_ASSERT(current != 0);
        BOOST_FOREACH(ast::statement const& s, x)
        {
            if (!(*this)(s))
                return false;
        }
        return true;
    }

    bool compiler::operator()(ast::if_statement const& x)
    {
        BOOST_ASSERT(current != 0);
        if (!(*this)(x.condition))
            return false;
        current->op(op_jump_if, 0);                 // we shall fill this (0) in later
        std::size_t skip = current->size()-1;       // mark its position
        if (!(*this)(x.then))
            return false;
        (*current)[skip] = current->size()-skip;    // now we know where to jump to (after the if branch)

        if (x.else_)                                // We got an alse
        {
            (*current)[skip] += 2;                  // adjust for the "else" jump
            current->op(op_jump, 0);                // we shall fill this (0) in later
            std::size_t exit = current->size()-1;   // mark its position
            if (!(*this)(*x.else_))
                return false;
            (*current)[exit] = current->size()-exit;// now we know where to jump to (after the else branch)
        }

        return true;
    }

    bool compiler::operator()(ast::while_statement const& x)
    {
        BOOST_ASSERT(current != 0);
        std::size_t loop = current->size();         // mark our position
        if (!(*this)(x.condition))
            return false;
        current->op(op_jump_if, 0);                 // we shall fill this (0) in later
        std::size_t exit = current->size()-1;       // mark its position
        if (!(*this)(x.body))
            return false;
        current->op(op_jump,
            int(loop-1) - int(current->size()));    // loop back
        (*current)[exit] = current->size()-exit;    // now we know where to jump to (to exit the loop)
        return true;
    }

    bool compiler::operator()(ast::return_statement const& x)
    {
        if (void_return)
        {
            if (x.expr)
            {
                error_handler(x.id, "'void' function returning a value: ");
                return false;
            }
        }
        else
        {
            if (!x.expr)
            {
                error_handler(x.id, current_function_name + " function must return a value: ");
                return false;
            }
        }

        if (x.expr)
        {
            if (!(*this)(*x.expr))
                return false;
        }
        current->op(op_return);
        return true;
    }

    bool compiler::operator()(ast::function const& x)
    {
        void_return = x.return_type == "void";
        if (functions.find(x.function_name.name) != functions.end())
        {
            error_handler(x.function_name.id, "Duplicate function: " + x.function_name.name);
            return false;
        }
        boost::shared_ptr<code_gen::function>& p = functions[x.function_name.name];
        p.reset(new code_gen::function(code, x.args.size()));
        current = p.get();
        current_function_name = x.function_name.name;

        // op_stk_adj 0 for now. we'll know how many variables
        // we'll have later and add them
        current->op(op_stk_adj, 0);
        BOOST_FOREACH(ast::identifier const& arg, x.args)
        {
            current->add_var(arg.name);
        }

        if (!(*this)(x.body))
            return false;
        (*current)[1] = current->nvars();   // now store the actual number of variables
                                            // this includes the arguments
        return true;
    }

    bool compiler::operator()(ast::function_list const& x)
    {
        // Jump to the main function
        code.push_back(op_jump);
        code.push_back(0); // we will fill this in later when we finish compiling
                           // and we know where the main function is

        BOOST_FOREACH(ast::function const& f, x)
        {
            if (!(*this)(f))
            {
                code.clear();
                return false;
            }
        }
        // find the main function
        boost::shared_ptr<code_gen::function> p =
            find_function("main");

        if (!p) // main function not found
        {
            std::cerr << "Error: main function not defined" << std::endl;
            return false;
        }
        code[1] = p->get_address()-1; // jump to this (main function) address

        return true;
    }

    void compiler::print_assembler() const
    {
        typedef std::pair<std::string, boost::shared_ptr<code_gen::function> > pair;
        BOOST_FOREACH(pair const& p, functions)
        {
            std::cout << ";;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;" << std::endl;
            std::cout << p.second->get_address() << ": function " << p.first << std::endl;
            p.second->print_assembler();
        }
    }

    boost::shared_ptr<code_gen::function>
    compiler::find_function(std::string const& name) const
    {
        function_table::const_iterator i = functions.find(name);
        if (i == functions.end())
            return boost::shared_ptr<code_gen::function>();
        else
            return i->second;
    }
}}

