/*=============================================================================
    Copyright (c) 2011 Thomas Bernard
    http://spirit.sourceforge.net/

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
//[reference_includes
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_container.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/repository/include/qi_kwd.hpp>
#include <boost/spirit/repository/include/qi_keywords.hpp>
#include <iostream>
#include <string>
#include <cstdlib>
#include <iterator>
//]


// Data structure definitions

struct base_type {
    base_type(const std::string &name) : name(name)  {}
    std::string name;
 
    virtual std::ostream &output(std::ostream &os) const
    {
        os<<"Base : "<<name;
        return os;
    }
  
};

struct derived1 : public base_type {
    derived1(const std::string &name, unsigned int data1) : base_type(name), data1(data1)  {}
    unsigned int data1;

    virtual std::ostream &output(std::ostream &os) const
    {
        base_type::output(os);
        os<<", "<<data1;
        return os;
    }

};

struct derived2 : public base_type {
    derived2(const std::string &name, unsigned int data2) : base_type(name), data2(data2)  {}
    unsigned int data2;
    virtual std::ostream &output(std::ostream &os) const
    {
        base_type::output(os);
        os<<", "<<data2;
        return os;
    }

};

struct derived3 : public derived2 {
    derived3(const std::string &name, unsigned int data2, double data3) : 
        derived2(name,data2),
        data3(data3)  {}
    double data3;

    virtual std::ostream &output(std::ostream &os) const
    {
        derived2::output(os);
        os<<", "<<data3;
        return os;
    }


};

std::ostream &operator<<(std::ostream &os, const base_type &obj)
{
    return obj.output(os);
}

BOOST_FUSION_ADAPT_STRUCT( base_type,
    (std::string, name)
)

BOOST_FUSION_ADAPT_STRUCT( derived1,
    (std::string , name)
    (unsigned int , data1)
)
BOOST_FUSION_ADAPT_STRUCT( derived2,
    (std::string , name)
    (unsigned int, data2)
)
BOOST_FUSION_ADAPT_STRUCT( derived3,
    (std::string , name)
    (unsigned int, data2)
    (double, data3)
)
//]

int
main()
{
   
     
        using boost::spirit::repository::qi::kwd;
        using boost::spirit::qi::inf;
        using boost::spirit::ascii::space_type; 
        using boost::spirit::ascii::char_;
        using boost::spirit::qi::double_;
        using boost::spirit::qi::int_;
        using boost::spirit::qi::rule;
        using boost::spirit::_val;
        using boost::spirit::_1;
        using boost::spirit::_2;
        using boost::spirit::_3;
    
       
        //Rule declarations
        rule<const char *, std::string(), space_type> parse_string;
        rule<const char *, std::vector<base_type*>(), space_type> kwd_rule; 
        
        // Our string parsing helper
        parse_string   %= '"'> *(char_-'"') > '"';
        
        namespace phx=boost::phoenix;    
        //[ kwd rule
        kwd_rule = 
            kwd("derived1")[ ('=' > parse_string > int_ ) [phx::push_back(_val,phx::new_<derived1>(_1,_2))] ]
          / kwd("derived2")[ ('=' > parse_string > int_ )  [phx::push_back(_val,phx::new_<derived2>(_1,_2))]]
          / kwd("derived3")[ ('=' > parse_string > int_ > double_)  [phx::push_back(_val,phx::new_<derived3>(_1,_2,_3))] ]
          ;
        //]
        
    using boost::spirit::qi::phrase_parse;
    using boost::spirit::qi::ascii::space;

    // The result vector
    std::vector<base_type*> result;
    
    char const input[]="derived2 = \"object1\" 10 derived3= \"object2\" 40 20.0 ";
    char const* f(input);
    char const* l(f + strlen(f));
    
    if (phrase_parse(f, l, kwd_rule, space,result) && (f == l))
        std::cout << "ok" << std::endl;
    else
        std::cout << "fail" << std::endl;

    using namespace boost::phoenix::arg_names;
    std::for_each(result.begin(),result.end(),std::cout<<*arg1<<std::endl);
    // Clean up the vector of pointers
    std::for_each(result.begin(),result.end(),phx::delete_(arg1));
    return 0;
}
