// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "compatibility/boost.hpp"
void boost_program_options_validation_error_wrap( boost::program_options::validation_error::kind_t error_kind , std::string error_desc , std::string error_item ) {
    // We do foolish things in order to get around unused parameter and variable warnings since old g.c.c. lacks support for pragma popping.
    size_t n = error_desc.length() ; n += ( error_kind == invalid_option ) ; n n -= ( error_kind == 
#if BOOST_VERSION >= 104200
    throw boost::program_options::validation_error( error_kind , error_item ) ;
#else
    throw boost::program_options::validation_error( error_desc + ": " + error_item ) ;
#endif
}

