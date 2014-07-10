// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef GEO_VINCENTY_HPP_
#define GEO_VINCENTY_HPP_

#include <utility>

class ellipsoid_spec_t;

typedef std::pair<double, double> lat_lon_point_t;

/* An implementation of Vincenty's formula.
 * Based on the description on
 * http://en.wikipedia.org/w/index.php?title=Vincenty%27s_formulae&oldid=607167872
 */

// Returns the ellispoidal distance between p1 and p2 on e (in meters).
double vincenty_distance(const lat_lon_point_t &p1,
                         const lat_lon_point_t &p2,
                         const ellipsoid_spec_t &e);

// TODO! Implement the direct problem so we can construct surface circles

#endif  // GEO_VINCENTY_HPP_
