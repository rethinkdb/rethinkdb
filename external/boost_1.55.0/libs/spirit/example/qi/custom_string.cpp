//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  This example demonstrates the steps needed to integrate a custom container
//  type to be usable with Spirit.Qi. It shows how to utilize the QString data
//  type as a Qi attribute.

//  Note: you need to have Qt4 installed on your system and you have to make 
//        sure your compiler finds the includes and your linker finds the 
//        proper libraries.

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>

#include <Qt/qstring.h>

namespace boost { namespace spirit { namespace traits
{
    // Make Qi recognize QString as a container
    template <> struct is_container<QString> : mpl::true_ {};

    // Expose the container's (QString's) value_type
    template <> struct container_value<QString> : mpl::identity<QChar> {};

    // Define how to insert a new element at the end of the container (QString)
    template <>
    struct push_back_container<QString, QChar>
    {
        static bool call(QString& c, QChar const& val)
        {
            c.append(val);
            return true;
        }
    };
}}}

///////////////////////////////////////////////////////////////////////////////
namespace client
{
    template <typename Iterator>
    bool parse_qstring(Iterator first, Iterator last, QString& t)
    {
        using boost::spirit::qi::char_;
        using boost::spirit::ascii::space;
        using boost::spirit::qi::phrase_parse;

        bool r = phrase_parse(first, last, +char_, space, t);
        if (!r || first != last) // fail if we did not get a full match
            return false;

        return r;
    }
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    std::cout << "/////////////////////////////////////////////////////////\n\n";
    std::cout << "\t\tParsing into a QString from Spirit...\n\n";
    std::cout << "/////////////////////////////////////////////////////////\n\n";

    std::cout << "Give me a complex number of the form r or (r) or (r,i) \n";
    std::cout << "Type [q or Q] to quit\n\n";

    std::string str;
    while (getline(std::cin, str))
    {
        if (str.empty() || str[0] == 'q' || str[0] == 'Q')
            break;

        QString t;
        if (client::parse_qstring(str.begin(), str.end(), t))
        {
            std::cout << "-------------------------\n";
            std::cout << "Parsing succeeded\n";
            std::cout << "got: " << t.toStdString() << std::endl;
            std::cout << "\n-------------------------\n";
        }
        else
        {
            std::cout << "-------------------------\n";
            std::cout << "Parsing failed\n";
            std::cout << "-------------------------\n";
        }
    }

    std::cout << "Bye... :-) \n\n";
    return 0;
}


