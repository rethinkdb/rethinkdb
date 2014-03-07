// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2010-2013 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2012-2013 Adam Wulkiewicz, Lodz, Poland.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//
#ifndef RAPIDXML_UTIL_HPP
#define RAPIDXML_UTIL_HPP


#include <string>
#include <rapidxml.hpp>

class xml_doc : public rapidxml::xml_document<>
{
    public :
        xml_doc(const char* xml)
        {
            // Make a copy because rapidxml destructs string
            m_copy = new char[strlen(xml) + 1];
            strcpy(m_copy, xml);
            this->parse<0>(m_copy);
        };
        virtual ~xml_doc()
        {
            delete[] m_copy;
        }
    private :
        char* m_copy;

};

inline std::string get_attribute(rapidxml::xml_node<>* node, const char* name)
{
    rapidxml::xml_attribute<> *attr = node->first_attribute(name);
    std::string value;
    if (attr)
    {
        value = attr->value();
    }
    return value;
}

inline void get_contents(rapidxml::xml_node<>* node, std::string& contents)
{
    if (node != NULL)
    {
        if (node->type() == rapidxml::node_element)
        {
            //std::cout << "ELEMENT: " << node->name() << "=" << node->value() << std::endl;
        }
        else if (node->type() == rapidxml::node_data)
        {
            contents += node->value();
            //std::cout << "DATA: " << node->name() << "=" << node->value() << std::endl;
        }
        else
        {
            //std::cout << "OTHER: " << node->name() << "=" << node->value() << std::endl;
        }
        get_contents(node->first_node(), contents);
        get_contents(node->next_sibling(), contents);
    }
}

#endif // RAPIDXML_UTIL_HPP
