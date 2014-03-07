//  Boost CRC example program file  ------------------------------------------//

//  Copyright 2003 Daryle Walker.  Use, modification, and distribution are
//  subject to the Boost Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or a copy at <http://www.boost.org/LICENSE_1_0.txt>.)

//  See <http://www.boost.org/libs/crc/> for the library's home page.

//  Revision History
//  17 Jun 2003  Initial version (Daryle Walker)

#include <boost/crc.hpp>  // for boost::crc_32_type

#include <cstdlib>    // for EXIT_SUCCESS, EXIT_FAILURE
#include <exception>  // for std::exception
#include <fstream>    // for std::ifstream
#include <ios>        // for std::ios_base, etc.
#include <iostream>   // for std::cerr, std::cout
#include <ostream>    // for std::endl


// Redefine this to change to processing buffer size
#ifndef PRIVATE_BUFFER_SIZE
#define PRIVATE_BUFFER_SIZE  1024
#endif

// Global objects
std::streamsize const  buffer_size = PRIVATE_BUFFER_SIZE;


// Main program
int
main
(
    int           argc,
    char const *  argv[]
)
try
{
    boost::crc_32_type  result;

    for ( int i = 1 ; i < argc ; ++i )
    {
        std::ifstream  ifs( argv[i], std::ios_base::binary );

        if ( ifs )
        {
            do
            {
                char  buffer[ buffer_size ];

                ifs.read( buffer, buffer_size );
                result.process_bytes( buffer, ifs.gcount() );
            } while ( ifs );
        }
        else
        {
            std::cerr << "Failed to open file '" << argv[i] << "'."
             << std::endl;
        }
    }

    std::cout << std::hex << std::uppercase << result.checksum() << std::endl;
    return EXIT_SUCCESS;
}
catch ( std::exception &e )
{
    std::cerr << "Found an exception with '" << e.what() << "'." << std::endl;
    return EXIT_FAILURE;
}
catch ( ... )
{
    std::cerr << "Found an unknown exception." << std::endl;
    return EXIT_FAILURE;
}
