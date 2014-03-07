# copyright John Maddock 2005
# Use, modification and distribution are subject to the 
# Boost Software License, Version 1.0. (See accompanying file 
# LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#
# locate all the header dependencies:
header=""
#
# locate all the source files:
src=link_test.cpp

boost_version=$(grep 'define.*BOOST_LIB_VERSION' ../../../../boost/version.hpp | sed 's/.*"\([^"]*\)".*/\1/')







