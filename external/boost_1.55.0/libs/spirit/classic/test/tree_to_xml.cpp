/*=============================================================================
    Copyright (c) 2001-2007 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include <boost/detail/lightweight_test.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/spirit/include/classic_core.hpp> 
#include <boost/spirit/include/classic_ast.hpp> 
#include <boost/spirit/include/classic_tree_to_xml.hpp> 

#include <iostream>
#include <fstream>
#include <string>

using namespace BOOST_SPIRIT_CLASSIC_NS; 

///////////////////////////////////////////////////////////////////////////////
struct calculator : public grammar<calculator>
{
    static const int integerID = 1;
    static const int factorID = 2;
    static const int termID = 3;
    static const int expressionID = 4;

    template <typename ScannerT>
    struct definition
    {
        definition(calculator const& /*self*/)
        {
            //  Start grammar definition
            integer     =   leaf_node_d[ lexeme_d[
                                (!ch_p('-') >> +digit_p)
                            ] ];

            factor      =   integer
                        |   inner_node_d[ch_p('(') >> expression >> ch_p(')')]
                        |   (root_node_d[ch_p('-')] >> factor);

            term        =   factor >>
                            *(  (root_node_d[ch_p('*')] >> factor)
                              | (root_node_d[ch_p('/')] >> factor)
                            );

            expression  =   term >>
                            *(  (root_node_d[ch_p('+')] >> term)
                              | (root_node_d[ch_p('-')] >> term)
                            );
            //  End grammar definition

            // turn on the debugging info.
            BOOST_SPIRIT_DEBUG_RULE(integer);
            BOOST_SPIRIT_DEBUG_RULE(factor);
            BOOST_SPIRIT_DEBUG_RULE(term);
            BOOST_SPIRIT_DEBUG_RULE(expression);
        }

        rule<ScannerT, parser_context<>, parser_tag<expressionID> >   expression;
        rule<ScannerT, parser_context<>, parser_tag<termID> >         term;
        rule<ScannerT, parser_context<>, parser_tag<factorID> >       factor;
        rule<ScannerT, parser_context<>, parser_tag<integerID> >      integer;

        rule<ScannerT, parser_context<>, parser_tag<expressionID> > const&
        start() const { return expression; }
    };
};

///////////////////////////////////////////////////////////////////////////////
/// this is a Boost.IoStreams source device usable to create a istream on 
/// top of a random access container (i.e. vector<>)
template<typename Container>
class container_device 
{
public:
    typedef typename Container::value_type char_type;
    typedef boost::iostreams::sink_tag category;
    
    container_device(Container& container) 
      : container_(container), pos_(0)
    {}

    /// Write up to n characters to the underlying data sink into the 
    /// buffer s, returning the number of characters written
    std::streamsize write(const char_type* s, std::streamsize n)
    {
        std::streamsize result = 0;
        if (pos_ != container_.size()) {
            std::streamsize amt = 
                static_cast<std::streamsize>(container_.size() - pos_);
            std::streamsize result = (std::min)(n, amt);
            std::copy(s, s + result, container_.begin() + pos_);
            pos_ += result;
        }
        if (result < n) {
            container_.insert(container_.end(), s, s + n);
            pos_ = container_.size();
        }
        return n;
    }

    Container& container() { return container_; }
    
private:
    typedef typename Container::size_type size_type;
    Container& container_;
    size_type pos_;
};

///////////////////////////////////////////////////////////////////////////////
#define EXPECTED_XML_OUTPUT "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\
<!DOCTYPE parsetree SYSTEM \"parsetree.dtd\">\n\
<!-- 1+2 -->\n\
<parsetree version=\"1.0\">\n\
    <parsenode>\n\
        <value>+</value>\n\
        <parsenode>\n\
            <value>1</value>\n\
        </parsenode>\n\
        <parsenode>\n\
            <value>2</value>\n\
        </parsenode>\n\
    </parsenode>\n\
</parsetree>\n"

#define EXPECTED_XML_OUTPUT_WIDE BOOST_PP_CAT(L, EXPECTED_XML_OUTPUT)

bool test(wchar_t const *text)
{
    typedef std::basic_string<wchar_t>::iterator iterator_t; 
    typedef tree_match<iterator_t> parse_tree_match_t; 
    typedef parse_tree_match_t::tree_iterator iter_t; 

    std::basic_string<wchar_t> input(text); 
    calculator calc; 
    tree_parse_info<iterator_t> ast_info = 
        ast_parse(iterator_t(input.begin()), iterator_t(input.end()), 
            calc >> end_p, space_p); 

    std::basic_string<wchar_t> out;
    {
        typedef container_device<std::basic_string<wchar_t> > device_type;
        boost::iostreams::stream<device_type> outsink(out);
        basic_tree_to_xml<wchar_t>(outsink, ast_info.trees, input); 
    }
    return out == EXPECTED_XML_OUTPUT_WIDE;
} 

bool test(char const *text)
{
    typedef std::string::iterator iterator_t; 
    typedef tree_match<iterator_t> parse_tree_match_t; 
    typedef parse_tree_match_t::tree_iterator iter_t; 

    std::string input(text); 
    calculator calc; 
    tree_parse_info<iterator_t> ast_info = 
        ast_parse(iterator_t(input.begin()), iterator_t(input.end()), 
            calc >> end_p, space_p); 

    std::string out;
    {
        typedef container_device<std::string> device_type;
        boost::iostreams::stream<device_type> outsink(out);
        basic_tree_to_xml<char>(outsink, ast_info.trees, input); 
    }
    return out == EXPECTED_XML_OUTPUT;
} 

int main() 
{ 
    BOOST_TEST(test("1+2"));
    if (std::has_facet<std::ctype<wchar_t> >(std::locale()))
    {
        BOOST_TEST(test(L"1+2"));
    }
    return boost::report_errors();
}
