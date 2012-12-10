// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "compatibility/boost.hpp"

#if BOOST_VERSION >= 104200
boost::program_options::validation_error::kind_t boost_program_options_validation_error_type_to_kind(
     enum boost_program_options_validation_error_type input
) {
    // This function converts from the internal enum to the boost enum.
    switch ( input ) {
        case multiple_values_not_allowed :
             return boost::program_options::validation_error::multiple_values_not_allowed ; break ;
        case at_least_one_value_required :
             return boost::program_options::validation_error::at_least_one_value_required ; break ;
        case invalid_bool_value :
             return boost::program_options::validation_error::invalid_bool_value ; break ;
        case invalid_option_value :
             return boost::program_options::validation_error::invalid_option_value ; break ;
        case invalid_option :
             return boost::program_options::validation_error::invalid_option ; break ;
        default :
             // This seems a decent if not desirable fallback.
             return boost::program_options::validation_error::invalid_option ; break ;
    }
    // We have a return value here so as to help older compilers to figure things out.
    return boost::program_options::validation_error::invalid_option ;
}
#endif

std::string boost_program_options_validation_error_type_to_string(
    enum boost_program_options_validation_error_type input
) {
    // This function converts from the internal enum to a string.
    switch ( input ) {
        case multiple_values_not_allowed : return STR_multiple_values_not_allowed ; break ;
        case at_least_one_value_required : return STR_at_least_one_value_required ; break ;
        case invalid_bool_value : return STR_invalid_bool_value ; break ;
        case invalid_option_value : return STR_invalid_option_value ; break ;
        case invalid_option : return STR_invalid_option ; break ;
        default : return "Unknown error" ; break ;
    }
    return "Error handling error" ;
}

void boost_program_options_validation_error_wrap(
    enum boost_program_options_validation_error_type error_type ,
    std::string error_item
) {
#if BOOST_VERSION >= 104200
    throw boost::program_options::validation_error(
        boost_program_options_validation_error_type_to_kind( error_type ) , error_item ) ;
#else
    throw boost::program_options::validation_error(
        boost_program_options_validation_error_type_to_string( error_type ) + ": " + error_item ) ;
#endif
}

