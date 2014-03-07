#include <cmath>
#include <boost/function.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/operator.hpp>
#include <boost/phoenix/stl/cmath.hpp>
#include <boost/detail/lightweight_test.hpp>

int main()
{
    double eps = 0.000001;
    using namespace boost::phoenix::arg_names;
    boost::function<bool(double, double)> f = fabs(_1 - _2) < eps;

    BOOST_TEST(f(0.0, 0 * eps));
    BOOST_TEST(!f(0.0, eps));
}
