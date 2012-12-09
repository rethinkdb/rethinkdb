// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef COMPATIBILITY_BOOST_HPP_
#define COMPATIBILITY_BOOST_HPP_
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fstream>  // NOLINT(readability/streams). Needed for use with program_options.  Sorry.

#include "errors.hpp"
#include <boost/program_options.hpp>

void boost_program_options_validation_error_wrap( boost::program_options::validation_error::kind_t error_kind , std::string error_desc , std::string error_item ) ;

#endif

