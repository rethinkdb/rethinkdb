/* Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
 distribution is subject to the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or copy at 
 http://www.boost.org/LICENSE_1_0.txt) 
 */
#ifndef HEADER_TEST_H
#define HEADER_TEST_H

#include <map>
#include <string>

namespace header_test {
    
enum choice { red, blue };

inline std::string choice_str(choice c)
{
    std::map<choice, std::string> choice_map;
    choice_map[red] = "red";
    choice_map[blue] = "blue";
    return choice_map[c];
}

struct C
{
    choice c;

    std::string get()
    {
        return choice_str(c);
    }
};

// test the exclusion of the following

struct ForwardDeclared; // should be excluded automatically
struct A {};
void foo();
enum bar { value };

}

#endif
