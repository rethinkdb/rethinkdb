//  (C) Copyright Eric Niebler 2006.
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <iomanip>
#include <fstream>
#include <deque>
#include <sstream>
#include <stdexcept>
#include <iterator>
#include "./regex_comparison.hpp"

//
// globals:
//
bool time_boost = false;
bool time_greta = false;
bool time_safe_greta = false;
bool time_dynamic_xpressive = false;
bool time_static_xpressive = false;
//bool time_posix = false;
//bool time_pcre = false;

bool test_matches = false;
bool test_short_twain = false;
bool test_long_twain = false;

std::string xml_out_file;
std::string xml_contents;
std::list<results> result_list;

int handle_argument(const std::string& what)
{
    if(what == "-b")
        time_boost = true;
    else if(what == "-g")
        time_greta = true;
    else if(what == "-gs")
        time_safe_greta = true;
    else if(what == "-dx")
        time_dynamic_xpressive = true;
    else if(what == "-sx")
        time_static_xpressive = true;
    //else if(what == "-posix")
    //   time_posix = true;
    //else if(what == "-pcre")
    //   time_pcre = true;
    else if(what == "-all")
    {
        time_boost = true;
        time_greta = true;
        time_safe_greta = true;
        time_dynamic_xpressive = true;
        time_static_xpressive = true;
        //time_posix = true;
        //time_pcre = true;
    }
    else if(what == "-test-matches")
        test_matches = true;
    else if(what == "-test-short-twain")
        test_short_twain = true;
    else if(what == "-test-long-twain")
        test_long_twain = true;
    else if(what == "-test-all")
    {
        test_matches = true;
        test_short_twain = true;
        test_long_twain = true;
    }
    else if((what == "-h") || (what == "--help"))
        return show_usage();
    else if((what[0] == '-') || (what[0] == '/'))
    {
        std::cerr << "Unknown argument: \"" << what << "\"" << std::endl;
        return 1;
    }
    else if(xml_out_file.size() == 0)
    {
        xml_out_file = what;
    }
    else
    {
        std::cerr << "Unexpected argument: \"" << what << "\"" << std::endl;
        return 1;
    }
    return 0;
}

int show_usage()
{
    std::cout <<
        "Usage\n"
        "xprperf [-h] [library options] [test options] [xml_output_file]\n"
        "   -h        Show help\n\n"
        "   library options:\n"
        "      -b     Apply tests to boost library\n"
        //"      -ba    Apply tests to boost library with a custom allocator\n"
        //"      -be    Apply tests to experimental boost library\n"
        //"      -g     Apply tests to GRETA library\n"
        //"      -gs    Apply tests to GRETA library (in non-recursive mode)\n"
        "      -dx    Apply tests to dynamic xpressive library\n"
        "      -sx    Apply tests to static xpressive library\n"
        //"      -posix Apply tests to POSIX library\n"
        //"      -pcre  Apply tests to PCRE library\n"
        "      -all   Apply tests to all libraries\n\n"
        "   test options:\n"
        "      -test-matches      Test short matches\n"
        "      -test-short-twain  Test short searches\n"
        "      -test-long-twain   Test long searches\n"
        "      -test-all          Test everthing\n";
    return 1;
}

void load_file(std::string& text, const char* file)
{
    std::deque<char> temp_copy;
    std::ifstream is(file);
    if(!is.good())
    {
        std::string msg("Unable to open file: \"");
        msg.append(file);
        msg.append("\"");
        throw std::runtime_error(msg);
    }
    std::istreambuf_iterator<char> it(is);
    std::copy(it, std::istreambuf_iterator<char>(), std::back_inserter(temp_copy));
    text.erase();
    text.reserve(temp_copy.size());
    text.append(temp_copy.begin(), temp_copy.end());
}

struct xml_double
{
    double d_;
    xml_double( double d ) : d_(d) {}
    friend std::ostream & operator<<( std::ostream & out, xml_double const & xd )
    {
        std::ostringstream tmp;
        tmp << std::setprecision(out.precision()) << xd.d_;
        std::string str = tmp.str();
        std::string::size_type i = str.find( '-' );
        if( i != std::string::npos )
            str.replace( i, 1, "&#8209;" );
        return out << str;
    }
};

void print_result(std::ostream& os, double time, double best)
{
    static const char* suffixes[] = {"s", "ms", "us", "ns", "ps", };

    if(time < 0)
    {
        os << "<entry>NA</entry>";
        return;
    }
    double rel = time / best;
    bool highlight = ((rel > 0) && (rel < 1.1));
    unsigned suffix = 0;
    while(time < 0)
    {
        time *= 1000;
        ++suffix;
    }
    os << "<entry>";
    if(highlight)
        os << "<phrase role=\"highlight\">";
    if(rel <= 1000)
        os << std::setprecision(3) << xml_double(rel);
    else
        os << (int)rel;
    os << "<para/>(";
    if(time <= 1000)
        os << std::setprecision(3) << xml_double(time);
    else
        os << (int)time;
    os << suffixes[suffix] << ")";
    if(highlight)
        os << "</phrase>";
    os << "</entry>";
}

void output_xml_results(bool show_description, const std::string& title, const std::string& filename)
{
    std::stringstream os;
    // Generate the copyright and license on the output file
    os << "<!--\n"
          " Copyright 2004 Eric Niebler.\n"
          "\n"
          " Distributed under the Boost Software License, Version 1.0.\n"
          " (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)\n"
          "-->\n";

    if(result_list.size())
    {
        // calculate the number of columns in this table
        int num_cols = 1 + show_description + time_greta + time_safe_greta 
            + time_dynamic_xpressive + time_static_xpressive + time_boost;

        //
        // start by outputting the table header:
        //
        os << "<informaltable frame=\"all\">\n";
        os << "<bridgehead renderas=\"sect4\">"
              "<phrase role=\"table-title\">" << title << "</phrase>"
              "</bridgehead>\n";
        os << "<tgroup cols=\"" << num_cols << "\">\n";
        os << "<thead>\n";
        os << "<row>\n";

        if(time_static_xpressive)   os << "<entry>static xpressive</entry>";
        if(time_dynamic_xpressive)  os << "<entry>dynamic xpressive</entry>";
        if(time_greta)              os << "<entry>GRETA</entry>";
        if(time_safe_greta)         os << "<entry>GRETA<para/>(non-recursive mode)</entry>";
        if(time_boost)              os << "<entry>Boost</entry>";
        //if(time_posix)              os << "<entry>POSIX</entry>";
        //if(time_pcre)               os << "<entry>PCRE</entry>";
        if(show_description)        os << "<entry>Text</entry>";
        os << "<entry>Expression</entry>";
        os << "\n</row>\n";
        os << "</thead>\n";
        os << "<tbody>\n";

        //
        // Now enumerate through all the test results:
        //
        std::list<results>::const_iterator first, last;
        first = result_list.begin();
        last = result_list.end();
        while(first != last)
        {
            os << "<row>\n";
            if(time_static_xpressive)   print_result(os, first->static_xpressive_time, first->factor);
            if(time_dynamic_xpressive)  print_result(os, first->dynamic_xpressive_time, first->factor);
            if(time_greta)              print_result(os, first->greta_time, first->factor);
            if(time_safe_greta)         print_result(os, first->safe_greta_time, first->factor);
            if(time_boost)              print_result(os, first->boost_time, first->factor);
            //if(time_posix)              print_result(os, first->posix_time, first->factor);
            //if(time_pcre)               print_result(os, first->pcre_time, first->factor);
            if(show_description)        os << "<entry>" << first->description << "</entry>";
            os << "<entry><literal>" << first->expression << "</literal></entry>";
            os << "\n</row>\n";
            ++first;
        }
        
        os << "</tbody>\n"
              "</tgroup>\n"
              "</informaltable>\n";

        result_list.clear();
    }
    else
    {
        os << "<para><emphasis>Results not available...</emphasis></para>\n";
    }

    std::ofstream file(filename.c_str());
    file << os.str();
}
