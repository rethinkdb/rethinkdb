// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef COMPATIBILITY_BOOST_HPP_
#define COMPATIBILITY_BOOST_HPP_
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fstream>  // NOLINT(readability/streams). Needed for use with program_options.  Sorry.
#include <string>

#include "errors.hpp"
#include <boost/program_options.hpp>

const std::string STR_multiple_values_not_allowed = "Multiple values not allowed" ;
const std::string STR_at_least_one_value_required = "At least one value required" ;
const std::string STR_invalid_bool_value = "Invalid bool value" ;
const std::string STR_invalid_option_value = "Invalid option value" ;
const std::string STR_invalid_option = "Invalid option" ;

enum boost_program_options_validation_error_type {
    multiple_values_not_allowed = 30 , 
    at_least_one_value_required ,
    invalid_bool_value , 
    invalid_option_value,
    invalid_option
} ;

#if BOOST_VERSION >= 104200
boost::program_options::validation_error::kind_t boost_program_options_validation_error_type_to_kind(
     enum boost_program_options_validation_error_type input
) ;
#endif

std::string boost_program_options_validation_error_type_to_string(
    enum boost_program_options_validation_error_type input
) ;

void boost_program_options_validation_error_wrap(
    enum boost_program_options_validation_error_type error_type ,
    std::string error_item
) ;

#endif

