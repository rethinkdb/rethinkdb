//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#include <boost/locale.hpp>
#include <iostream>

#include <ctime>

int main()
{
    using namespace boost::locale;
    using namespace std;
    generator gen;
    locale loc=gen(""); 
    // Create system default locale

    locale::global(loc); 
    // Make it system global
    
    cout.imbue(loc);
    // Set as default locale for output
    
    cout <<format("Today {1,date} at {1,time} we had run our first localization example") % time(0) 
          <<endl;
   
    cout<<"This is how we show numbers in this locale "<<as::number << 103.34 <<endl; 
    cout<<"This is how we show currency in this locale "<<as::currency << 103.34 <<endl; 
    cout<<"This is typical date in the locale "<<as::date << std::time(0) <<endl;
    cout<<"This is typical time in the locale "<<as::time << std::time(0) <<endl;
    cout<<"This is upper case "<<to_upper("Hello World!")<<endl;
    cout<<"This is lower case "<<to_lower("Hello World!")<<endl;
    cout<<"This is title case "<<to_title("Hello World!")<<endl;
    cout<<"This is fold case "<<fold_case("Hello World!")<<endl;
   
}

// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
