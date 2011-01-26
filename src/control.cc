#include "control.hpp"
#include "logger.hpp"

control_map_t &get_control_map() {
    /* Getter function so that we can be sure that control_list is initialized before it is needed,
    as advised by the C++ FAQ. Otherwise, a control_t  might be initialized before the control list
    was initialized. */
    
    static control_map_t control_map;
    return control_map;
}

std::string control_exec(std::string key) {
    std::string res;

    for (control_map_t::iterator it = get_control_map().find(key); it != get_control_map().end() && (*it).first == key; it++)
        res += (*it).second->call();

    return res;
}

control_t::control_t(std::string key) 
    : key(key)
{ //TODO locks @jdoliner
    get_control_map().insert(std::pair<std::string, control_t*>(key, this));
}

control_t::~control_t() {
    for (control_map_t::iterator it = get_control_map().find(key); it != get_control_map().end(); it++)
        if ((*it).second == this)
            get_control_map().erase(it);
}

/* Example of how to add a control */
struct hi_t : public control_t
{
private:
    int counter;
public:
    hi_t(std::string key)
        : control_t(key), counter(0)
    {}
    std::string call() {
        counter++;
        if (counter < 3)
            return std::string("Salutations, user.\n");
        else if (counter < 4)
            return std::string("Say hi again, I dare you.\n");
        else
            return std::string("Base QPS decreased by 100,000\n");
    }
};

hi_t hi(std::string("hi"));
