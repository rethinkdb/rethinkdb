
#include <boost/range.hpp>
#include <boost/range/irange.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/phoenix.hpp>
#include <boost/detail/lightweight_test.hpp>

using namespace boost::phoenix::arg_names;
using namespace boost::adaptors;

int foo() { return 5; }

int main()
{
    BOOST_TEST((*boost::begin(boost::irange(0,5) | transformed( arg1)) == 0));

    boost::report_errors();
}
