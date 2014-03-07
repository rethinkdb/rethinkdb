//  Copyright (c) 2001-2009 Hartmut Kaiser
//  Copyright (c) 2009 Carl Barron
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef MATLIB_H_05102009
#define MATLIB_H_05102009
#include <boost/spirit/include/lex.hpp>
#include <vector>
#include <string>

struct set_lexer_state
{
    std::string state;
    set_lexer_state(const std::string &a):state(a){}
    template <class Iterator,class Context>
    void operator () (Iterator const&, Iterator const&
      , BOOST_SCOPED_ENUM(boost::spirit::lex::pass_flags)&, std::size_t
      , Context &ctx) const
    {
        ctx.set_state_name(state.c_str());
    }
};

struct store_double
{
    std::vector<double> &out;
    store_double(std::vector<double> &a):out(a){}
    template <class Iterator,class LexerContext>
    void operator () (Iterator const& start, Iterator const& end
      , BOOST_SCOPED_ENUM(boost::spirit::lex::pass_flags)&, std::size_t
      , LexerContext &) const
    {
        std::string work(start, end);
        out.push_back(std::atof(work.c_str()));
    }

private:
    // silence MSVC warning C4512: assignment operator could not be generated
    store_double& operator= (store_double const&);
};

struct add_row
{
    std::vector<std::vector<double> > &matrix;
    std::vector<double> &row;

    add_row(std::vector<std::vector<double> > &a,std::vector<double> &b)
        :matrix(a),row(b) {}
    template <class Iterator,class Context>
    void operator () (Iterator const&, Iterator const&
      , BOOST_SCOPED_ENUM(boost::spirit::lex::pass_flags)&, std::size_t
      , Context &ctx) const
    {
        matrix.push_back(std::vector<double>());
        matrix.back().swap(row);
        ctx.set_state_name("A");
    }

private:
    // silence MSVC warning C4512: assignment operator could not be generated
    add_row& operator= (add_row const&);
};

template <class Lexer>
struct matlib_tokens : boost::spirit::lex::lexer<Lexer>
{
    matlib_tokens(std::vector<std::vector<double> > &a)
      : matrix(a)
    {
        typedef boost::spirit::lex::token_def<> token_def_;

        this->self.add_pattern("REAL1", "[0-9]+(\\.[0-9]*)?");
        this->self.add_pattern("REAL2", "\\.[0-9]+");

        number = "[-+]?({REAL1}|{REAL2})([eE][-+]?[0-9]+)?";

        this->self 
            =   token_def_('[') [set_lexer_state("A")]
            ;

        this->self("A") 
            =   token_def_('[') [set_lexer_state("B")]
            |   ','
            |   token_def_(']') [set_lexer_state("INITIAL")]
            ;

        this->self("B") 
            =   number [store_double(row)]
            |   ','
            |   token_def_(']') [add_row(matrix,row)]
            ;
    }

    boost::spirit::lex::token_def<> number;
    std::vector<std::vector<double> > &matrix;
    std::vector<double> row;
};

#endif

