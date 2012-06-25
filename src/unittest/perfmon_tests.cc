#include "unittest/gtest.hpp"

#include "perfmon/perfmon.hpp"
#include <math.h>
#include <stdint.h>

namespace unittest {

TEST(PerfmonTest, StddevComputation) {
    typedef stddev_t t;

    {   t stats;
        EXPECT_EQ(0, stats.datapoints());
        EXPECT_TRUE(isnan(stats.mean()));
        EXPECT_TRUE(isnan(stats.standard_variance()));
        EXPECT_TRUE(isnan(stats.standard_deviation()));
    }

    {   t stats;
        stats.add(5.0);

        EXPECT_EQ(1, stats.datapoints());
        // shouldn't have any floating-point error yet
        EXPECT_EQ(5.0, stats.mean());
        EXPECT_EQ(0.0, stats.standard_variance());
        EXPECT_EQ(0.0, stats.standard_deviation());
    }

    // Uniform distribution over the integers [-N, N] for many values of N <=2000
    for (int_fast64_t N = 10; N <= 2000; N *= 1.1) {
        t stats;
        int_fast64_t datapoints = N * 2 + 1;
        int_fast64_t squared_dists = 0;
        for (int64_t i = -N; i <= N; ++i) {
            squared_dists += i * i;
            stats.add(float(i));
        }

        float var = float(squared_dists) / datapoints;
        // acceptable relative difference. picked out of a hat.
        static const float reldiff = 0.00005;
        EXPECT_EQ(datapoints, stats.datapoints());
        EXPECT_FLOAT_EQ(0.0, stats.mean());
        EXPECT_NEAR(var, stats.standard_variance(), var * reldiff);
        EXPECT_NEAR(sqrt(var), stats.standard_deviation(), sqrt(var) * reldiff);
    }
}

}  // namespace unittest
