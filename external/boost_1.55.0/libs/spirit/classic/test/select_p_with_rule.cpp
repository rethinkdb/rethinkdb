/*=============================================================================
    Copyright (c) 2004 Vyacheslav E. Andrejev
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <iostream>
 
#define PHOENIX_LIMIT 2
#define BOOST_SPIRIT_SELECT_LIMIT 2
#define BOOST_SPIRIT_RULE_SCANNERTYPE_LIMIT 2
 
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_lists.hpp>
#include <boost/spirit/include/classic_select.hpp>
 
using namespace BOOST_SPIRIT_CLASSIC_NS;
using namespace std;
 
struct format_grammar : public grammar<format_grammar>
{
    template <typename ScannerT>
    struct definition
    {
        definition(format_grammar const& /*self*/)
        {
            descriptor_list =
                    list_p(format_descriptor, ch_p(','))
                ;

            format_descriptor = 
                    select_p(E_descriptor, EN_descriptor)
                ;
    
            E_descriptor = // E[w[.d][Ee]]
                    lexeme_d
                    [
                        (ch_p('E') - (str_p("EN"))) 
                    >> !(
                            min_limit_d(1u)[uint_p] 
                        >> !(ch_p('.') >> uint_p) 
                        >> !(ch_p('E') >> min_limit_d(1u)[uint_p])
                        )
                    ]
                ;
 
            EN_descriptor = // EN[w[.d][Ee]]
                    lexeme_d
                    [
                        str_p("EN") 
                    >> !(
                            min_limit_d(1u)[uint_p] 
                        >> !(ch_p('.') >> uint_p) 
                        >> !(ch_p('E') >> min_limit_d(1u)[uint_p])
                        )
                    ]
                ;
        }
 
        rule<ScannerT> descriptor_list;
        rule<ScannerT> format_descriptor;
        rule<ScannerT> E_descriptor;
        rule<ScannerT> EN_descriptor;
 
        rule<ScannerT> const& start() const 
        { 
            return descriptor_list; 
        }
    };
};
 
int main(int argc, char* argv[])
{
    format_grammar grammar;
    const char* format = "E2, EN15.7, E20.10E3, E, EN";
 
    parse_info<> pi = parse(format, grammar, blank_p);
 
    if (pi.full) {
        cout << "Test concluded successful" << endl;
        return 0;
    } 
    else {
        BOOST_SPIRIT_ASSERT(false);     // Test fails
        return -1;
    }
}
