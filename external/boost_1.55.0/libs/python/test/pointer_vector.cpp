// Copyright Joel de Guzman 2005-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <vector>

using namespace boost::python;

class Abstract
{
public:   
    virtual ~Abstract() {}; // silence compiler warningsa
    virtual std::string    f() =0;
};

class Concrete1 : public Abstract
{
public:   
    virtual std::string    f()    { return "harru"; }
};

typedef std::vector<Abstract*>   ListOfObjects;

class DoesSomething
{
public:
    DoesSomething()    {}
   
    ListOfObjects   returnList()    
    {
        ListOfObjects lst; 
        lst.push_back(new Concrete1()); return lst; 
    }
};

BOOST_PYTHON_MODULE(pointer_vector_ext)
{       
class_<Abstract, boost::noncopyable>("Abstract", no_init)
    .def("f", &Abstract::f)
    ;

class_<ListOfObjects>("ListOfObjects")
   .def( vector_indexing_suite<ListOfObjects>() )  
    ;

class_<DoesSomething>("DoesSomething")
    .def("returnList", &DoesSomething::returnList)
    ;
}


