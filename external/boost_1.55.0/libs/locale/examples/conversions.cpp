//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#include <boost/locale.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <iostream>

#include <ctime>



int main()
{
    using namespace boost::locale;
    using namespace std;
    // Create system default locale
    generator gen;
    locale loc=gen(""); 
    locale::global(loc); 
    cout.imbue(loc);

    
    cout<<"Correct case conversion can't be done by simple, character by character conversion"<<endl;
    cout<<"because case conversion is context sensitive and not 1-to-1 conversion"<<endl;
    cout<<"For example:"<<endl;
    cout<<"   German grüßen correctly converted to "<<to_upper("grüßen")<<", instead of incorrect "
                    <<boost::to_upper_copy(std::string("grüßen"))<<endl;
    cout<<"     where ß is replaced with SS"<<endl;
    cout<<"   Greek ὈΔΥΣΣΕΎΣ is correctly converted to "<<to_lower("ὈΔΥΣΣΕΎΣ")<<", instead of incorrect "
                    <<boost::to_lower_copy(std::string("ὈΔΥΣΣΕΎΣ"))<<endl;
    cout<<"     where Σ is converted to σ or to ς, according to position in the word"<<endl;
    cout<<"Such type of conversion just can't be done using std::toupper that work on character base, also std::toupper is "<<endl;
    cout<<"not even applicable when working with variable character length like in UTF-8 or UTF-16 limiting the correct "<<endl;
    cout<<"behavior to unicode subset BMP or ASCII only"<<endl;
   
}

// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4

// boostinspect:noascii
