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
#include <boost/algorithm/string/case_conv.hpp>

using namespace std;
using namespace boost;

int main()
{  
    cout << "* Case Conversion Example *" << endl << endl;

    string str1("AbCdEfG");
    vector<char> vec1( str1.begin(), str1.end() );
    
    // Convert vector of chars to lower case
    cout << "lower-cased copy of vec1: ";
    to_lower_copy( ostream_iterator<char>(cout), vec1 );
    cout << endl;

    // Conver string str1 to upper case ( copy the input )
    cout << "upper-cased copy of str1: " << to_upper_copy( str1 ) << endl;

    // Inplace conversion
    to_lower( str1 );
    cout << "lower-cased str1: " << str1 << endl;

    cout << endl;

    return 0;
}
