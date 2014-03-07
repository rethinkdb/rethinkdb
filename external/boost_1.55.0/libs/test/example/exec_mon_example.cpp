//  (C) Copyright Gennadiy Rozental 2003-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

#include <boost/test/prg_exec_monitor.hpp>
#include <boost/test/execution_monitor.hpp>
#include <boost/test/utils/basic_cstring/io.hpp>

#include <iostream>

struct my_exception1
{
    explicit    my_exception1( int res_code ) : m_res_code( res_code ) {}

    int         m_res_code;
};

struct my_exception2
{
    explicit    my_exception2( int res_code ) : m_res_code( res_code ) {}

    int         m_res_code;
};

namespace {

class dangerous_call {
public:
    dangerous_call( int argc ) : m_argc( argc ) {}
    
    int operator()()
    {
        // here we perform some operation under monitoring that could throw my_exception
        if( m_argc < 2 )
            throw my_exception1( 23 );
        if( m_argc > 3 )
            throw my_exception2( 45 );
        else if( m_argc > 2 )
            throw "too many args";

        return 1;
    }

private:
    // Data members  
    int     m_argc;
};

void translate_my_exception1( my_exception1 const& ex )
{
    std::cout << "Caught my_exception1(" << ex.m_res_code << ")"<< std::endl;   
}

void translate_my_exception2( my_exception2 const& ex )
{
    std::cout << "Caught my_exception2(" << ex.m_res_code << ")"<< std::endl;   
}

} // local_namespace

int
cpp_main( int argc , char *[] )
{ 
    ::boost::execution_monitor ex_mon;

    ex_mon.register_exception_translator<my_exception1>( &translate_my_exception1 );
    ex_mon.register_exception_translator<my_exception2>( &translate_my_exception2 );

    try {
        ex_mon.execute( ::boost::unit_test::callback0<int>( dangerous_call( argc ) ) );
    }
    catch ( boost::execution_exception const& ex ) {
        std::cout << "Caught exception: " << ex.what() << std::endl;
    }

    return 0;
}

// EOF
