//  tiny XML test program  ---------------------------------------------------//

//  Copyright Beman Dawes 2002.  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "tiny_xml.hpp"

#include <iostream>

int main()
{
  boost::tiny_xml::element_ptr tree( boost::tiny_xml::parse( std::cin ) );
  boost::tiny_xml::write( *tree, std::cout );
  return 0;
}

