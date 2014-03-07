//	https://svn.boost.org/trac/boost/ticket/5521 
//		claims a linker error for this.

#include <boost/signal.hpp>
#include <boost/signals/connection.hpp>

struct HelloWorld 
{
  void operator()() const 
  { 
    std::cout << "Hello, World!" << std::endl;
  } 
};


int main ( int argc, char *argv [] ) {
	boost::signal<void ()> sig;
	boost::signals::scoped_connection c1, c2;
	
	c1 = sig.connect ( HelloWorld ());
	std::swap ( c1, c2 );
	return 0;
	}
