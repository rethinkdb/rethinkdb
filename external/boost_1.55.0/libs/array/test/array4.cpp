/* example for using class array<>
 * (C) Copyright Nicolai M. Josuttis 2001.
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#include <algorithm>
#include <functional>
#include <string>
#include <iostream>
#include <boost/array.hpp>

int main()
{
    // array of arrays of seasons
    boost::array<boost::array<std::string,4>,2> seasons_i18n = {
        { { { "spring", "summer", "autumn", "winter", } },
          { { "Fruehling", "Sommer", "Herbst", "Winter" } }
        }
    };

    // for any array of seasons print seasons
    for (unsigned i=0; i<seasons_i18n.size(); ++i) {
        boost::array<std::string,4> seasons = seasons_i18n[i];
        for (unsigned j=0; j<seasons.size(); ++j) {
            std::cout << seasons[j] << " ";
        }
        std::cout << std::endl;
    }

    // print first element of first array
    std::cout << "first element of first array: "
              << seasons_i18n[0][0] << std::endl;

    // print last element of last array
    std::cout << "last element of last array: "
              << seasons_i18n[seasons_i18n.size()-1][seasons_i18n[0].size()-1]
              << std::endl;

    return 0;  // makes Visual-C++ compiler happy
}

