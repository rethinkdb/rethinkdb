// Copyright David Abrahams 2005. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/def.hpp>
#include <boost/python/class.hpp>
#include <boost/python/wrapper.hpp>
#include <boost/python/module.hpp>
#include <boost/python/implicit.hpp>

#include <memory>

struct data
{
    virtual ~data() {}; // silence compiler warnings
    virtual int id() const
    {
        return 42;
    }
};
    
std::auto_ptr<data> create_data()
{ 
    return std::auto_ptr<data>( new data ); 
}

void do_nothing( std::auto_ptr<data>& ){}

    
namespace bp = boost::python;

struct data_wrapper : data, bp::wrapper< data >
{
    data_wrapper(data const & arg )
    : data( arg )
      , bp::wrapper< data >()
    {}

    data_wrapper()
    : data()
      , bp::wrapper< data >()
    {}

    virtual int id() const
    {
        if( bp::override id = this->get_override( "id" ) )
            return bp::call<int>(id.ptr()); // id();
        else
            return data::id(  );
    }
    
    virtual int default_id(  ) const
    {
        return this->data::id( );
    }

};

BOOST_PYTHON_MODULE(wrapper_held_type_ext)
{
    bp::class_< data_wrapper, std::auto_ptr< data > >( "data" )    
        .def( "id", &data::id, &::data_wrapper::default_id );

    bp::def( "do_nothing", &do_nothing );
    bp::def( "create_data", &create_data );
}    

#include "module_tail.cpp"
