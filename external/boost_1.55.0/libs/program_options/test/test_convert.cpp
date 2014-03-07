// Copyright Vladimir Prus 2002-2004.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <cstring>
#include <cassert>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <boost/progress.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>

#include <boost/program_options/detail/convert.hpp>
#include <boost/program_options/detail/utf8_codecvt_facet.hpp>

#include "minitest.hpp"

using namespace std;

string file_content(const string& filename)
{
    ifstream ifs(filename.c_str());
    assert(ifs);
    
    stringstream ss;
    ss << ifs.rdbuf();
    
    return ss.str();
}

// A version of from_8_bit which does not use functional object, for
// performance comparison.
std::wstring from_8_bit_2(const std::string& s, 
                          const codecvt<wchar_t, char, mbstate_t>& cvt)
{
    std::wstring result;


    std::mbstate_t state = std::mbstate_t();
    
    const char* from = s.data();
    const char* from_end = s.data() + s.size();
    // The interace of cvt is not really iterator-like, and it's
    // not possible the tell the required output size without the conversion.
    // All we can is convert data by pieces.
    while(from != from_end) {
            
        // std::basic_string does not provide non-const pointers to the data,
        // so converting directly into string is not possible.
        wchar_t buffer[32];
            
        wchar_t* to_next = buffer;
        // Try to convert remaining input.
        std::codecvt_base::result r = 
            cvt.in(state, from, from_end, from, buffer, buffer + 32, to_next);
        
        if (r == std::codecvt_base::error)
            throw logic_error("character conversion failed");
        // 'partial' is not an error, it just means not all source characters
        // we converted. However, we need to check that at least one new target
        // character was produced. If not, it means the source data is 
        // incomplete, and since we don't have extra data to add to source, it's
        // error.
        if (to_next == buffer)
            throw logic_error("character conversion failed");

        // Add converted characters
        result.append(buffer, to_next);
    }

    return result;        
}


void test_convert(const std::string& input, 
                  const std::string& expected_output)
{
    boost::program_options::detail::utf8_codecvt_facet facet;
    
    std::wstring output;
    { 
        boost::progress_timer t;
        for (int i = 0; i < 10000; ++i)
            output = boost::from_8_bit(
                input,
                facet);
    }

    {
        boost::progress_timer t;
        for (int i = 0; i < 10000; ++i)
            output = from_8_bit_2(
                input,
                facet);
    }

    BOOST_CHECK(output.size()*2 == expected_output.size());

    for(unsigned i = 0; i < output.size(); ++i) {

        {
            unsigned low = output[i];
            low &= 0xFF;
            unsigned low2 = expected_output[2*i];
            low2 &= 0xFF;
            BOOST_CHECK(low == low2);
        }
        {        
            unsigned high = output[i];
            high >>= 8;
            high &= 0xFF;
            unsigned high2 = expected_output[2*i+1];            
            BOOST_CHECK(high == high2);
        }
    }

    string ref = boost::to_8_bit(output, facet);

    BOOST_CHECK(ref == input);
}

int main(int ac, char* av[])
{       
    std::string input = file_content("utf8.txt");
    std::string expected = file_content("ucs2.txt");

    test_convert(input, expected);
    
    if (ac > 1) {
        cout << "Trying to convert the command line argument\n";
    
        locale::global(locale(""));
        std::wstring w = boost::from_local_8_bit(av[1]);
 
        cout << "Got something, printing decimal code point values\n";
        for (unsigned i = 0; i < w.size(); ++i) {
            cout << (unsigned)w[i] << "\n";
        }
        
    }
    
    return 0;
}
