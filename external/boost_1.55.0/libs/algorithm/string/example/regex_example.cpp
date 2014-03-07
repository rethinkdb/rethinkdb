//  Boost string_algo library example file  ---------------------------------//

//  Copyright Pavol Droba 2002-2003. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <string>
#include <iostream>
#include <iterator>
#include <boost/regex.hpp>
#include <boost/algorithm/string/regex.hpp>

using namespace std;
using namespace boost;

int main()
{  
    cout << "* Regex Example *" << endl << endl;

    string str1("abc__(456)__123__(123)__cde");

    // Replace all substrings matching (digit+) 
    cout << 
        "replace all (digit+) in str1 with #digit+# :" <<
        replace_all_regex_copy( str1, regex("\\(([0-9]+)\\)"), string("#$1#") ) << endl;
    
    // Erase all substrings matching (digit+) 
    cout << 
        "remove all sequences of letters from str1 :" <<
        erase_all_regex_copy( str1, regex("[[:alpha:]]+") ) << endl;

    // in-place regex transformation
    replace_all_regex( str1, regex("_(\\([^\\)]*\\))_"), string("-$1-") );
    cout << "transformad str1: " << str1 << endl;

    cout << endl;

    return 0;
}
