//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  The main purpose of this example is to show how a single container type can
//  be formatted using different output grammars. 

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

#include <cmath>

using namespace boost::spirit;

///////////////////////////////////////////////////////////////////////////////
// This policy allows to use printf style formatting specifiers for Karma
// floating point generators. This policy understands the following format:
//
//  The format string must conform to the following format, otherwise a 
//  std::runtime_error will be thrown:
//
//      %[flags][fill][width][.precision]type
//
//  where:
//      flags (only one possible):
//          +:      Always denote the sign '+' or '-' of a number 
//          -:      Left-align the output 
//      fill:
//          0:      Uses 0 instead of spaces to left-fill a fixed-length field
//      width:
//          number: Left-pad the output with spaces until it is at least number 
//                  characters wide. if number has a leading '0', that is 
//                  interpreted as a 'fill', the padding is done with '0' 
//                  characters instead of spaces.
//      precision:
//          number: Causes the decimal portion of the output to be expressed 
//                  in at least number digits
//      type (only one possible):
//          e:      force scientific notation, with a lowercase "e"
//          E:      force scientific notation, with a uppercase "E"
//          f:      floating point format
//          g:      use %e or %f, whichever is shorter
//          G:      use %E or %f, whichever is shorter
//

///////////////////////////////////////////////////////////////////////////////
// define a data structure and a corresponding parser to hold the formatting 
// information extracted from the format specification string
namespace client
{
    struct format_data
    {
        char flag;
        char fill;
        int width;
        int precision;
        char type;
    };
}

// We need to tell fusion about our format_data struct
// to make it a first-class fusion citizen
BOOST_FUSION_ADAPT_STRUCT(
    client::format_data,
    (char, flag)
    (char, fill)
    (int, width)
    (int, precision)
    (char, type)
)

namespace client
{
    ///////////////////////////////////////////////////////////////////////////
    // Grammar for format specification string as described above
    template <typename Iterator>
    struct format_grammar : qi::grammar<Iterator, format_data()>
    {
        format_grammar() : format_grammar::base_type(format)
        {
            using qi::uint_;
            using qi::attr;
            using ascii::char_;
            using ascii::no_case;

            format %= '%' >> flags >> fill >> width >> prec >> type;

            // default flags is right aligned
            flags  = char_('+') | char_('-') | attr(' ');
            fill   = char_('0') | attr(' ');     // default fill is space
            width  = uint_ | attr(-1);
            prec   = '.' >> uint_ | attr(3);     // default is 3 digits
            type   = no_case[char_('e')] | char_('f') | no_case[char_('g')];
        };

        qi::rule<Iterator, format_data()> format;
        qi::rule<Iterator, char()> flags;
        qi::rule<Iterator, char()> fill;
        qi::rule<Iterator, int()> width;
        qi::rule<Iterator, int()> prec;
        qi::rule<Iterator, char()> type;
    };
}

///////////////////////////////////////////////////////////////////////////////
// real_policies implementation allowing to use a printf style format 
// specification for Karma floating pointing number generators
template <typename T>
struct format_policies : karma::real_policies<T>
{
    typedef karma::real_policies<T> base_policy_type;

    ///////////////////////////////////////////////////////////////////////////
    //  This real_policies implementation requires the output_iterator to 
    //  implement buffering and character counting. This needs to be reflected
    //  in the properties exposed by the generator
    typedef boost::mpl::int_<
        karma::generator_properties::countingbuffer
    > properties;

    ///////////////////////////////////////////////////////////////////////////
    format_policies(char const* fmt = "%f")
    {
        char const* last = fmt;
        while (*last)
            last++;

        client::format_grammar<char const*> g;
        if (!qi::parse(fmt, last, g, format_))
            throw std::runtime_error("bad format string");
    }

    ///////////////////////////////////////////////////////////////////////////
    //  returns the overall format: scientific or fixed
    int floatfield(T n) const
    {
        if (format_.type == 'e' || format_.type == 'E')
            return base_policy_type::fmtflags::scientific;

        if (format_.type == 'f')
            return base_policy_type::fmtflags::fixed;

        BOOST_ASSERT(format_.type == 'g' || format_.type == 'G');
        return this->base_policy_type::floatfield(n);
    }

    ///////////////////////////////////////////////////////////////////////////
    //  returns whether to emit a sign even for non-negative numbers
    bool const force_sign(T) const
    {
        return format_.flag == '+';
    }

    ///////////////////////////////////////////////////////////////////////////
    //  returns the number of required digits for the fractional part
    unsigned precision(T) const
    {
        return format_.precision;
    }

    ///////////////////////////////////////////////////////////////////////////
    //  emit the decimal dot
    template <typename OutputIterator>
    static bool dot (OutputIterator& sink, T n, unsigned precision) 
    {
        // don't print the dot if no fractional digits are to be emitted
        if (precision == 0)
            return true;
        return base_policy_type::dot(sink, n, precision);
    }

    template <typename CharEncoding, typename Tag, typename OutputIterator>
    bool exponent (OutputIterator& sink, long n) const
    {
        if (format_.type == 'E' || format_.type == 'G') {
            // print exponent symbol in upper case
            return this->base_policy_type::
                template exponent<char_encoding::ascii, tag::upper>(sink, n);
        }
        return this->base_policy_type::
            template exponent<CharEncoding, Tag>(sink, n);
    }

    ///////////////////////////////////////////////////////////////////////////
    //  this gets called by the numeric generators at the top level, it allows
    //  to do alignment and other high level things
    template <typename Inserter, typename OutputIterator, typename Policies>
    bool call (OutputIterator& sink, T n, Policies const& p) const
    {
        bool r = false;
        if (format_.flag == '-') {    // left align
            // wrap the given output iterator to allow counting
            karma::detail::enable_counting<OutputIterator> counting(sink);

            // first generate the actual floating point number
            r = Inserter::call_n(sink, n, p);

            // pad the output until the max width is reached
            while(r && int(counting.count()) < format_.width) 
                r = karma::generate(sink, ' ');
        }
        else {                        // right align
            // wrap the given output iterator to allow left padding
            karma::detail::enable_buffering<OutputIterator> buffering(
                sink, format_.width);

            // first generate the actual floating point number
            {
                karma::detail::disable_counting<OutputIterator> nocounting(sink);
                r = Inserter::call_n(sink, n, p);
            }

            buffering.disable();    // do not perform buffering any more

            // generate the left padding
            karma::detail::enable_counting<OutputIterator> counting(
                sink, buffering.buffer_size());
            while(r && int(counting.count()) < format_.width) 
                r = karma::generate(sink, format_.fill);

            // copy the buffered output to the target output iterator
            if (r) 
                buffering.buffer_copy();
        }
        return r;
    }

    client::format_data format_;
};

///////////////////////////////////////////////////////////////////////////////
// This is the generator usable in any Karma output format expression, it needs
// to be utilized as
//
//    generate(sink, real("%6.3f"), 3.1415926536);    // prints: ' 3.142'
//
// and it supports the format specification as described above.
typedef karma::real_generator<double, format_policies<double> > real;

///////////////////////////////////////////////////////////////////////////////
int main()
{
    std::cout << "/////////////////////////////////////////////////////////////\n\n";
    std::cout << "A format driven floating point number generator for Spirit...\n\n";
    std::cout << "/////////////////////////////////////////////////////////////\n\n";

    std::cout << "Give me a printf style format\n";
    std::cout << "Type [enter] to quit\n\n";

    std::string str;
    while (getline(std::cin, str))
    {
        if (str.empty())
            break;

        try {
            std::string generated;
            std::back_insert_iterator<std::string> sink(generated);
            if (!karma::generate(sink, real(str.c_str()), 4*std::atan(1.0)))
            {
                std::cout << "-------------------------\n";
                std::cout << "Generating failed\n";
                std::cout << "-------------------------\n";
            }
            else
            {
                std::cout << ">" << generated << "<\n";
            }
        }
        catch (std::runtime_error const&) {
            std::cout << "-------------------------\n";
            std::cout << "Invalid format specified!\n";
            std::cout << "-------------------------\n";
        }
    }

    return 0;
}

