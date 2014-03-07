/*=============================================================================
    Copyright (c) 2003 Vaclav Vesely
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
//
//  This example demonstrates the lazy_p parser. You should read
//  "The Lazy Parser" in the documentation.
//
//  We want to parse nested blocks of numbers like this:
//
//  dec {
//      1 2 3
//      bin {
//          1 10 11
//      }
//      4 5 6
//  }
//
//  where the numbers in the "dec" block are wrote in the decimal system and
//  the numbers in the "bin" block are wrote in the binary system. We want
//  parser to return the overall sum.
//
//  To achive this when base ("bin" or "dec") is parsed, in semantic action
//  we store a pointer to the appropriate numeric parser in the closure
//  variable block.int_rule. Than, when we need to parse a number we use lazy_p
//  parser to invoke the parser stored in the block.int_rule pointer.
//
//-----------------------------------------------------------------------------
#include <boost/assert.hpp>
#include <boost/cstdlib.hpp>
#include <boost/spirit/include/phoenix1.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_symbols.hpp>
#include <boost/spirit/include/classic_attribute.hpp>
#include <boost/spirit/include/classic_dynamic.hpp>

using namespace boost;
using namespace BOOST_SPIRIT_CLASSIC_NS;
using namespace phoenix;

//-----------------------------------------------------------------------------
//  my grammar

struct my_grammar
    :   public grammar<my_grammar, parser_context<int> >
{
    // grammar definition
    template<typename ScannerT>
    struct definition
    {
        typedef rule<ScannerT> rule_t;
        typedef stored_rule<ScannerT, parser_context<int> > number_rule_t;

        struct block_closure;
        typedef boost::spirit::classic::closure<
            block_closure,
            int,
            typename number_rule_t::alias_t>
        closure_base_t;

        struct block_closure : closure_base_t
        {
            typename closure_base_t::member1 sum;
            typename closure_base_t::member2 int_rule;
        };

        // block rule type
        typedef rule<ScannerT, typename block_closure::context_t> block_rule_t;

        block_rule_t block;
        rule_t block_item;
        symbols<number_rule_t> base;

        definition(my_grammar const& self)
        {
            block =
                    base[
                        block.sum = 0,
                        // store a number rule in a closure member
                        block.int_rule = arg1
                    ]
                >>  "{"
                >>  *block_item
                >>  "}"
                ;

            block_item =
                    // use the stored rule
                    lazy_p(block.int_rule)[block.sum += arg1]
                |   block[block.sum += arg1]
                ;

            // bind base keywords and number parsers
            base.add
                ("bin", bin_p)
                ("dec", uint_p)
                ;
        }

        block_rule_t const& start() const
        {
            return block;
        }
    };
};

//-----------------------------------------------------------------------------

int main()
{
    my_grammar gram;
    parse_info<> info;

    int result;
    info = parse("bin{1 dec{1 2 3} 10}", gram[var(result) = arg1], space_p);
    BOOST_ASSERT(info.full);
    BOOST_ASSERT(result == 9);

    return exit_success;
}

//-----------------------------------------------------------------------------
