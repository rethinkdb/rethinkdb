// Copyright 2010-2012 RethinkDB, all rights reserved.

#include <stdint.h>
#include <math.h>

#include <cmath>  // for std::isnan -- read the comment below.

#include "perfmon/perfmon.hpp"
#include "unittest/gtest.hpp"

namespace unittest {

TEST(PerfmonTest, StddevComputation) {
    typedef stddev_t t;

    {
        t stats;
        EXPECT_EQ(0u, stats.datapoints());

        // We're getting an error on OS X that isnan isn't a recognized identifier (despite the man
        // page saying you just need to include <math.h>).  perfmon.cc uses it and it compiles, even
        // if you include all the same headers that it has, here.  So we use std::isnan here.  I
        // guess the gtest header affects things somehow.
        EXPECT_TRUE(std::isnan(stats.mean()));
        EXPECT_TRUE(std::isnan(stats.standard_variance()));
        EXPECT_TRUE(std::isnan(stats.standard_deviation()));
    }

    {
        t stats;
        stats.add(5.0);

        EXPECT_EQ(1u, stats.datapoints());
        // shouldn't have any floating-point error yet
        EXPECT_EQ(5.0, stats.mean());
        EXPECT_EQ(0.0, stats.standard_variance());
        EXPECT_EQ(0.0, stats.standard_deviation());
    }

    // Uniform distribution over the integers [-N, N] for many values of N <=2000
    for (int64_t N = 10; N <= 2000; N *= 1.1) {
        t stats;
        const uint64_t datapoints = N * 2 + 1;
        int64_t squared_dists = 0;
        for (int64_t i = -N; i <= N; ++i) {
            squared_dists += i * i;
            stats.add(static_cast<double>(i));
        }

        double var = static_cast<double>(squared_dists) / datapoints;
        // acceptable relative difference. picked out of a hat.
        static const double reldiff = 0.00005;
        EXPECT_EQ(datapoints, stats.datapoints());
        EXPECT_FLOAT_EQ(0.0, stats.mean());
        EXPECT_NEAR(var, stats.standard_variance(), var * reldiff);
        EXPECT_NEAR(sqrt(var), stats.standard_deviation(), sqrt(var) * reldiff);
    }
}

}  // namespace unittest
