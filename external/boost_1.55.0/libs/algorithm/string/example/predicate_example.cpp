//  Boost string_algo library example file  ---------------------------------//

//  Copyright Pavol Droba 2002-2003. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <string>
#include <iostream>
#include <functional>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/bind.hpp>


using namespace std;
using namespace boost;

int main()
{
    cout << "* Predicate Example *" << endl << endl;

    string str1("123xxx321");
    string str2("abc");

    // Check if str1 starts with '123'
    cout << "str1 starts with \"123\": " << 
        (starts_with( str1, string("123") )?"true":"false") << endl; 
    
    // Check if str1 ends with '123'
    cout << "str1 ends with \"123\": " << 
        (ends_with( str1, string("123") )?"true":"false") << endl; 

    // Check if str1 contains 'xxx'
    cout << "str1 contains \"xxx\": " << 
        (contains( str1, string("xxx") )?"true":"false") << endl; 


    // Check if str2 equals to 'abc'
    cout << "str2 equals \"abc\": " << 
        (equals( str2, string("abc") )?"true":"false") << endl; 


    // Classification functors and all predicate
    if ( all(";.,", is_punct() ) )
    {
        cout << "\";.,\" are all punctuation characters" << endl;  
    }

    // Classification predicates can be combined 
    if ( all("abcxxx", is_any_of("xabc") && !is_space() ) )
    {
        cout << "true" << endl;
    }

    cout << endl;

    return 0;
}   
