// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "geo/vincenty.hpp"

#define _USE_MATH_DEFINES
#include <cmath>

#include "geo/ellipsoid.hpp"
#include "geo/exceptions.hpp"


inline double deg_to_rad(double d) {
    return d * M_PI / 180.0;
}

// The inverse problem of Vincenty's formulas
// See http://en.wikipedia.org/w/index.php?title=Vincenty%27s_formulae&oldid=607167872
double vincenty_distance(const lat_lon_point_t &p1,
                         const lat_lon_point_t &p2,
                         const ellipsoid_spec_t &e) {
    // TODO! Check each term for corner/error cases etc.
    // TODO! Equal points yield nan.
    // TODO! It doesn't converge for e.g. 0, 0 - 45, 0

    const double u1 = atan((1.0f - e.flattening()) * tan(deg_to_rad(p1.first)));
    const double u2 = atan((1.0f - e.flattening()) * tan(deg_to_rad(p2.first)));
    // TODO! Do I have to mod this?
    const double l = deg_to_rad(p2.second) - deg_to_rad(p1.second);

    const int iteration_limit = 1000;

    bool converged = false;
    int iteration = 0;
    double lambda = l;
    double prev_lambda = lambda;
    double cos_sq_alpha;
    double sin_sigma, cos_sigma;
    double cos_2sigma_m;
    do {
        ++iteration;

        sin_sigma =
            sqrt(pow(cos(u2) * sin(lambda), 2)
                 + pow(cos(u1) * sin(u2) - sin(u1) * cos(u2) * cos(lambda), 2));
        cos_sigma = sin(u1) * sin(u2) + cos(u1) * cos(u2) * cos(lambda);
        double sin_alpha = (cos(u1) * cos(u2) * sin(lambda)) / sin_sigma;
        cos_sq_alpha = 1.0 - pow(sin_alpha, 2);
        cos_2sigma_m = cos_sigma - (2.0 * sin(u1) * sin(u2)) / cos_sq_alpha;
        double c = e.flattening() / 16.0 * cos_sq_alpha *
            (4.0 + e.flattening() * (4.0 - 3.0 * cos_sq_alpha));
        lambda = l + (1.0 - c) * e.flattening() * sin_alpha *
            (cos_2sigma_m + c * cos_sigma * (-1.0 + 2.0 * pow(cos_2sigma_m, 2)));

        // TODO! How to define convergence? Does this work?
        converged = prev_lambda == lambda;
        prev_lambda = lambda;
        // TODO! Compare with other open source implementations to make sure everything is correct
    } while (!converged && iteration < iteration_limit);
    if (!converged) {
        // TODO! Improve error message
        throw geo_exception_t(
            strprintf("Distance did not converge after %d iterations", iteration_limit));
    }

    double sigma = atan(sin_sigma / cos_sigma);
    double u_sq = cos_sq_alpha *
        (pow(e.equator_radius(), 2) - pow(e.poles_radius(), 2)) / pow(e.poles_radius(), 2);
    double a = 1.0 + u_sq / 16384.0 * (2096.0 + u_sq * (-768.0 + u_sq * (320.0 - 175.0 * u_sq)));
    double b = u_sq / 1024.0 * (256.0 + u_sq * (-128.0 + u_sq * (74.0 - 47.0 * u_sq)));
    double delta_sigma = b * sin_sigma *
        (cos_2sigma_m + 1/4 * b * (cos_sigma * (-1.0 + 2.0 * pow(cos_2sigma_m, 2)) -
                                   1/6 * b * cos_2sigma_m * (-3.0 + 4.0 * pow(sin_sigma, 2)) *
                                   (-3.0 + 4.0 * pow(cos_2sigma_m, 2))));
    double s = e.poles_radius() * a * (sigma - delta_sigma);

    return s;
}
