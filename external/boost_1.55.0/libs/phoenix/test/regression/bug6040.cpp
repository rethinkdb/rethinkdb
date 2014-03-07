#include <boost/phoenix.hpp>
#include <vector>
#include <algorithm>
#include <sstream>
 
int main()
{
    std::vector<unsigned char> data;
    using boost::phoenix::arg_names::_1;
    using boost::phoenix::static_cast_;
    std::ostringstream oss;
    oss << std::hex;
    std::for_each(data.begin(),data.end(), static_cast_<unsigned int>(_1) );
}
