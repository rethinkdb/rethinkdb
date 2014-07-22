// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "geo/ellipsoid.hpp"

#include "rpc/serialize_macros.hpp"

RDB_MAKE_SERIALIZABLE_2(ellipsoid_spec_t, a, f);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(ellipsoid_spec_t);
