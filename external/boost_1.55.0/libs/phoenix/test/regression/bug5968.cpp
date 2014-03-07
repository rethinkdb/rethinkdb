
#include <boost/phoenix.hpp>
#include <boost/signals2.hpp>
 
struct s
{
        bool f(int, bool) { return true; }
};
 
int main()
{
        s s_obj;
        boost::signals2::signal<bool (int, bool)> sig;
        sig.connect(
                boost::phoenix::bind(
                        &s::f, &s_obj,
                        boost::phoenix::placeholders::arg1,
                        boost::phoenix::placeholders::arg2));
}

