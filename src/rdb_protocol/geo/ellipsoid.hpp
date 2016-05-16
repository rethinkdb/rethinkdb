// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_GEO_ELLIPSOID_HPP_
#define RDB_PROTOCOL_GEO_ELLIPSOID_HPP_

#include "rpc/serialize_macros.hpp"

class ellipsoid_spec_t {
public:
    ellipsoid_spec_t() { } // For deserialization
    ellipsoid_spec_t(double _equator_radius, double _flattening)
        : equator_radius_(_equator_radius), flattening_(_flattening) { }

    // In meters
    inline double equator_radius() const {
        return equator_radius_;
    }
    // In meters
    inline double poles_radius() const {
        return (1.0 - flattening_) * equator_radius_;
    }
    // Flattening, see http://en.wikipedia.org/w/index.php?title=Flattening&oldid=602517763
    inline double flattening() const {
        return flattening_;
    }

    RDB_DECLARE_ME_SERIALIZABLE(ellipsoid_spec_t);

private:
    double equator_radius_;
    double flattening_;
};

// WGS 84 is a commonly used standard for earth geometry, see
// http://en.wikipedia.org/w/index.php?title=World_Geodetic_System&oldid=614370148
static const ellipsoid_spec_t WGS84_ELLIPSOID(6378137.0, 1.0/298.257223563);
static const ellipsoid_spec_t UNIT_SPHERE(1.0, 0.0);

#endif  // RDB_PROTOCOL_GEO_ELLIPSOID_HPP_
