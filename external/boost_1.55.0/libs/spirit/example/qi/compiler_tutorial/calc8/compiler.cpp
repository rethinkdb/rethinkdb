/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include "compiler.hpp"
#include "vm.hpp"
#include <boost/foreach.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/assert.hpp>
#include <boost/lexical_cast.hpp>
#include <set>

namespace client { namespace code_gen
{
    void program::op(int a)
    {
        code.push_back(a);
    }

    void program::op(int a, int b)
    {
        code.push_back(a);
        code.push_back(b);
    }

    void program::op(int a, int b, int c)
    {
        code.push_back(a);
        code.push_back(b);
        code.push_back(c);
    }

    int const* program::find_var(std::string const& name) const
    {
        std::map<std::string, int>::const_iterator i = variables.find(name);
        if (i == variables.end())
            return 0;
        return &i->second;
    }

    void program::add_var(std::string const& name)
    {
        std::size_t n = variables.size();
        variables[name] = n;
    }

    void program::print_variables(std::vector<int> const& stack) const
    {
        typedef std::pair<std::string, int> pair;
        BOOST_FOREACH(pair const& p, variables)
        {
            std::cout << "    " << p.first << ": " << stack[p.second] << std::endl;
        }
    }

    void program::print_assembler() const
    {
        std::vector<int>::const_iterator pc = code.begin();

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

        while (pc != code.end())
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

                case op_stk_adj:
                    line += "      op_stk_adj  ";
                    line += boost::lexical_cast<std::string>(*pc++);
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

        std::cout << "end:" << std::endl;
    }

    bool compiler::operator()(unsigned int x) const
    {
        program.op(op_int, x);
        return true;
    }

    bool compiler::operator()(bool x) const
    {
        program.op(x ? op_true : op_false);
        return true;
    }

    bool compiler::operator()(ast::variable const& x) const
    {
        int const* p = program.find_var(x.name);
        if (p == 0)
        {
            std::cout << x.id << std::endl;
            error_handler(x.id, "Undeclared variable: " + x.name);
            return false;
        }
        program.op(op_load, *p);
        return true;
    }

    bool compiler::operator()(ast::operation const& x) const
    {
        if (!boost::apply_visitor(*this, x.operand_))
            return false;
        switch (x.operator_)
        {
            case ast::op_plus: program.op(op_add); break;
            case ast::op_minus: program.op(op_sub); break;
            case ast::op_times: program.op(op_mul); break;
            case ast::op_divide: program.op(op_div); break;

            case ast::op_equal: program.op(op_eq); break;
            case ast::op_not_equal: program.op(op_neq); break;
            case ast::op_less: program.op(op_lt); break;
            case ast::op_less_equal: program.op(op_lte); break;
            case ast::op_greater: program.op(op_gt); break;
            case ast::op_greater_equal: program.op(op_gte); break;

            case ast::op_and: program.op(op_and); break;
            case ast::op_or: program.op(op_or); break;
            default: BOOST_ASSERT(0); return false;
        }
        return true;
    }

    bool compiler::operator()(ast::unary const& x) const
    {
        if (!boost::apply_visitor(*this, x.operand_))
            return false;
        switch (x.operator_)
        {
            case ast::op_negative: program.op(op_neg); break;
            case ast::op_not: program.op(op_not); break;
            case ast::op_positive: break;
            default: BOOST_ASSERT(0); return false;
        }
        return true;
    }

    bool compiler::operator()(ast::expression const& x) const
    {
        if (!boost::apply_visitor(*this, x.first))
            return false;
        BOOST_FOREACH(ast::operation const& oper, x.rest)
        {
            if (!(*this)(oper))
                return false;
        }
        return true;
    }

    bool compiler::operator()(ast::assignment const& x) const
    {
        if (!(*this)(x.rhs))
            return false;
        int const* p = program.find_var(x.lhs.name);
        if (p == 0)
        {
            std::cout << x.lhs.id << std::endl;
            error_handler(x.lhs.id, "Undeclared variable: " + x.lhs.name);
            return false;
        }
        program.op(op_store, *p);
        return true;
    }

    bool compiler::operator()(ast::variable_declaration const& x) const
    {
        int const* p = program.find_var(x.assign.lhs.name);
        if (p != 0)
        {
            std::cout << x.assign.lhs.id << std::endl;
            error_handler(x.assign.lhs.id, "Duplicate variable: " + x.assign.lhs.name);
            return false;
        }
        bool r = (*this)(x.assign.rhs);
        if (r) // don't add the variable if the RHS fails
        {
            program.add_var(x.assign.lhs.name);
            program.op(op_store, *program.find_var(x.assign.lhs.name));
        }
        return r;
    }

    bool compiler::operator()(ast::statement const& x) const
    {
        return boost::apply_visitor(*this, x);
    }

    bool compiler::operator()(ast::statement_list const& x) const
    {
        BOOST_FOREACH(ast::statement const& s, x)
        {
            if (!(*this)(s))
                return false;
        }
        return true;
    }

    bool compiler::operator()(ast::if_statement const& x) const
    {
        if (!(*this)(x.condition))
            return false;
        program.op(op_jump_if, 0);                  // we shall fill this (0) in later
        std::size_t skip = program.size()-1;        // mark its position
        if (!(*this)(x.then))
            return false;
        program[skip] = program.size()-skip;        // now we know where to jump to (after the if branch)

        if (x.else_)                                // We got an alse
        {
            program[skip] += 2;                     // adjust for the "else" jump
            program.op(op_jump, 0);                 // we shall fill this (0) in later
            std::size_t exit = program.size()-1;    // mark its position
            if (!(*this)(*x.else_))
                return false;
            program[exit] = program.size()-exit;    // now we know where to jump to (after the else branch)
        }

        return true;
    }

    bool compiler::operator()(ast::while_statement const& x) const
    {
        std::size_t loop = program.size();          // mark our position
        if (!(*this)(x.condition))
            return false;
        program.op(op_jump_if, 0);                  // we shall fill this (0) in later
        std::size_t exit = program.size()-1;        // mark its position
        if (!(*this)(x.body))
            return false;
        program.op(op_jump,
            int(loop-1) - int(program.size()));     // loop back
        program[exit] = program.size()-exit;        // now we know where to jump to (to exit the loop)
        return true;
    }

    bool compiler::start(ast::statement_list const& x) const
    {
        program.clear();
        // op_stk_adj 0 for now. we'll know how many variables we'll have later
        program.op(op_stk_adj, 0);

        if (!(*this)(x))
        {
            program.clear();
            return false;
        }
        program[1] = program.nvars(); // now store the actual number of variables
        return true;
    }
}}

