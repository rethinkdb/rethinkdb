// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef GEO_ELLIPSOID_HPP_
#define GEO_ELLIPSOID_HPP_

#include "rpc/serialize_macros.hpp"

class ellipsoid_spec_t {
public:
    ellipsoid_spec_t() { } // For deserialization
    ellipsoid_spec_t(double _a, double _f) : a(_a), f(_f) { }

    // In meters
    inline double equator_radius() const {
        return a;
    }
    // In meters
    inline double poles_radius() const {
        return (1.0 - f) * a;
    }
    // Flattening, see http://en.wikipedia.org/w/index.php?title=Flattening&oldid=602517763
    inline double flattening() const {
        return f;
    }

private:
    template<cluster_version_t W>
    friend void serialize(write_message_t *, const ellipsoid_spec_t &);
    template<cluster_version_t W>
    friend archive_result_t deserialize(read_stream_t *, ellipsoid_spec_t *);

    double a, f;
};
RDB_DECLARE_SERIALIZABLE(ellipsoid_spec_t);

static const ellipsoid_spec_t WGS84_ELLIPSOID(6378137.0, 1.0/298.257223563);
static const ellipsoid_spec_t UNIT_SPHERE(1.0, 0.0);

#endif  // GEO_ELLIPSOID_HPP_
