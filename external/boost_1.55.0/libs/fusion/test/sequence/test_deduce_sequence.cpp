
#include <boost/mpl/vector.hpp>
#include <boost/fusion/support.hpp>

typedef boost::fusion::traits::deduce_sequence < 

boost::mpl::vector<int, char> 

>::type seq1_t;


typedef boost::fusion::traits::deduce_sequence < 

boost::fusion::vector<int, char> 

>::type seq2_t;
