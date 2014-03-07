//  Boost string_algo library example file  ---------------------------------//

//  Copyright Pavol Droba 2002-2003. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <string>
#include <iostream>
#include <iterator>
//#include <boost/algorithm/string/replace.hpp>
//#include <boost/algorithm/string/erase.hpp>
//#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string.hpp>

//Following two includes contain second-layer function.
//They are already included by first-layer header

//#include <boost/algorithm/string/replace2.hpp>
//#include <boost/algorithm/string/find2.hpp>

using namespace std;
using namespace boost;

// uppercase formatter
/*
    Convert an input to upper case. 
    Note, that this formatter can be used only on std::string inputs.
*/
inline string upcase_formatter( 
    const iterator_range<string::const_iterator>& Replace )
{
    string Temp(Replace.begin(), Replace.end());
    to_upper(Temp);
    return Temp;
}

int main()
{  
    cout << "* Replace Example *" << endl << endl;

    string str1("abc___cde___efg");

    // Erase 6-9th characters from the string
    cout << "str1 without 6th to 9th character:" <<
        erase_range_copy( str1, make_iterator_range(str1.begin()+6, str1.begin()+9) ) << endl;

    // Replace 6-9th character with '+++'
    cout << "str1 with 6th to 9th character replaced with '+++': " << 
        replace_range_copy( 
            str1, make_iterator_range(str1.begin()+6, str1.begin()+9), "+++" ) << endl;

    cout << "str1 with 'cde' replaced with 'XYZ': ";
    
    // Replace first 'cde' with 'XYZ'. Modify the input
    replace_first_copy( ostream_iterator<char>(cout), str1, "cde", "XYZ" );
    cout << endl;
    
    // Replace all '___'
    cout << "str1 with all '___' replaced with '---': " << 
        replace_all_copy( str1, "___", "---" ) << endl;

    // Erase all '___'
    cout << "str1 without all '___': " << 
        erase_all_copy( str1, "___" ) << endl;

    // replace third and 5th occurrence of _ in str1
    // note that nth argument is 0-based
    replace_nth( str1, "_", 4, "+" );
    replace_nth( str1, "_", 2, "+" );

    cout << "str1 with third and 5th occurrence of _ replace: " << str1 << endl;

    // Custom formatter examples
    string str2("abC-xxxx-AbC-xxxx-abc");

    // Find string 'abc' ignoring the case and convert it to upper case
    cout << "Upcase all 'abc'(s) in the str2: " <<
        find_format_all_copy( 
            str2,
            first_finder("abc", is_iequal()), 
            upcase_formatter );
    
    cout << endl;

    return 0;
}
