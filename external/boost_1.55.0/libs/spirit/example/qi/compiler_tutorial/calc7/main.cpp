/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
///////////////////////////////////////////////////////////////////////////////
//
//  Now we'll introduce variables and assignment. This time, we'll also
//  be renaming some of the rules -- a strategy for a grander scheme
//  to come ;-)
//
//  This version also shows off grammar modularization. Here you will
//  see how expressions and statements are built as modular grammars.
//
//  [ JDG April 9, 2007 ]       spirit2
//  [ JDG February 18, 2011 ]   Pure attributes. No semantic actions.
//
///////////////////////////////////////////////////////////////////////////////

#include "statement.hpp"
#include "vm.hpp"
#include "compiler.hpp"

///////////////////////////////////////////////////////////////////////////////
//  Main program
///////////////////////////////////////////////////////////////////////////////
int
main()
{
    std::cout << "/////////////////////////////////////////////////////////\n\n";
    std::cout << "Statement parser...\n\n";
    std::cout << "/////////////////////////////////////////////////////////\n\n";
    std::cout << "Type some statements... ";
    std::cout << "An empty line ends input, compiles, runs and prints results\n\n";
    std::cout << "Example:\n\n";
    std::cout << "    var a = 123;\n";
    std::cout << "    var b = 456;\n";
    std::cout << "    var c = a + b * 2;\n\n";
    std::cout << "-------------------------\n";

    std::string str;
    std::string source;
    while (std::getline(std::cin, str))
    {
        if (str.empty())
            break;
        source += str + '\n';
    }

    typedef std::string::const_iterator iterator_type;
    iterator_type iter = source.begin();
    iterator_type end = source.end();

    client::vmachine vm;                                    // Our virtual machine
    client::code_gen::program program;                      // Our VM program
    client::ast::statement_list ast;                        // Our AST

    client::error_handler<iterator_type>
        error_handler(iter, end);                           // Our error handler
    client::parser::statement<iterator_type>
        parser(error_handler);                              // Our parser
    client::code_gen::compiler
        compile(program, error_handler);                    // Our compiler

    boost::spirit::ascii::space_type space;
    bool success = phrase_parse(iter, end, parser, space, ast);

    std::cout << "-------------------------\n";

    if (success && iter == end)
    {
        if (compile(ast))
        {
            std::cout << "Success\n";
            std::cout << "-------------------------\n";
            vm.execute(program());

            std::cout << "-------------------------\n";
            std::cout << "Assembler----------------\n\n";
            program.print_assembler();

            std::cout << "-------------------------\n";
            std::cout << "Results------------------\n\n";
            program.print_variables(vm.get_stack());
        }
        else
        {
            std::cout << "Compile failure\n";
        }
    }
    else
    {
        std::cout << "Parse failure\n";
    }

    std::cout << "-------------------------\n\n";
    return 0;
}


