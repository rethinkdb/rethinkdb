//  Boost string_algo library example file  ---------------------------------//

//  Copyright Pavol Droba 2002-2003. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <string>
#include <vector>
#include <iostream>
#include <iterator>
#include <functional>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/find_iterator.hpp>

using namespace std;
using namespace boost;

int main()
{  
    cout << "* Split Example *" << endl << endl;

    string str1("abc-*-ABC-*-aBc");

    cout << "Before: " << str1 << endl;

    // Find all 'abc' substrings (ignoring the case)
    // Create a find_iterator
    typedef find_iterator<string::iterator> string_find_iterator;
    for(string_find_iterator It=
            make_find_iterator(str1, first_finder("abc", is_iequal()));
        It!=string_find_iterator();
        ++It)
    {
        cout << copy_range<std::string>(*It) << endl;
        // shift all chars in the match by one
        transform( 
            It->begin(), It->end(), 
            It->begin(), 
            bind2nd( plus<char>(), 1 ) );
    }

    // Print the string now
    cout << "After: " << str1 << endl;
    
    // Split the string into tokens ( use '-' and '*' as delimiters )
    // We need copies of the input only, and adjacent tokens are compressed
    vector<std::string> ResultCopy;
    split(ResultCopy, str1, is_any_of("-*"), token_compress_on);

    for(unsigned int nIndex=0; nIndex<ResultCopy.size(); nIndex++)
    {
        cout << nIndex << ":" << ResultCopy[nIndex] << endl;
    };

    cout << endl;

    return 0;
}
