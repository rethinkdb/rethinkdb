// Copyright David Abrahams 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#include <boost/python/module.hpp>
#include <boost/assert.hpp>

#include <boost/python/def.hpp>
#include <boost/python/class.hpp>
#include <boost/python/str.hpp>
#define BOOST_ENABLE_ASSERT_HANDLER
#include <boost/assert.hpp>

using namespace boost::python;

object convert_to_string(object data)
{
    return str(data);
}

void work_with_string(object print)
{
    str data("this is a demo string");
    print(data.split(" "));
    print(data.split(" ",3));
    print(str("<->").join(data.split(" ")));
    print(data.capitalize());
    print('[' + data.center(30) + ']');
    print(data.count("t"));
#if PY_VERSION_HEX < 0x03000000
    print(data.encode("utf-8"));
    print(data.decode("utf-8"));
#else
    print(data.encode("utf-8").attr("decode")("utf-8"));
    print(data.encode("utf-8").attr("decode")("utf-8"));
#endif
    
    BOOST_ASSERT(!data.endswith("xx"));
    BOOST_ASSERT(!data.startswith("test"));
    
    print(data.splitlines());
    print(data.strip());
    print(data.swapcase());
    print(data.title());

    print("find");
    print(data.find("demo"));
    print(data.find("demo"),3,5);
    print(data.find(std::string("demo")));
    print(data.find(std::string("demo"),9));

    print("expandtabs");
    str tabstr("\t\ttab\tdemo\t!");
    print(tabstr.expandtabs());
    print(tabstr.expandtabs(4));
    print(tabstr.expandtabs(7L));

    print("operators");
    print( str("part1") + str("part2") );
//    print( str("a test string").slice(3,_) );
//    print( str("another test")[5] );

    print(data.replace("demo",std::string("blabla")));
    print(data.rfind("i",5));
    print(data.rindex("i",5));

    BOOST_ASSERT(!data.startswith("asdf"));
    BOOST_ASSERT(!data.endswith("asdf"));
    
    print(data.translate(str('a')*256));


    bool tmp = data.isalnum() || data.isalpha() || data.isdigit() || data.islower() ||
        data.isspace() || data.istitle() || data.isupper();
    (void)tmp; // ignored.
}
   

BOOST_PYTHON_MODULE(str_ext)
{
    def("convert_to_string",convert_to_string);
    def("work_with_string",work_with_string);
}

#include "module_tail.cpp"
