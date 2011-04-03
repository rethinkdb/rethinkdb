#include "control.hpp"
#include "logger.hpp"
#include "errors.hpp"
#include "concurrency/multi_wait.hpp"

control_map_t &get_control_map() {
    /* Getter function so that we can be sure that control_list is initialized before it is needed,
    as advised by the C++ FAQ. Otherwise, a control_t  might be initialized before the control list
    was initialized. */
    
    static control_map_t control_map;
    return control_map;
}

spinlock_t &get_control_lock() {
    /* To avoid static initialization fiasco */
    
    static spinlock_t lock;
    return lock;
}

std::string control_t::exec(int argc, char **argv) {
    if (argc == 0) return help();
    std::string command = argv[0];

    control_map_t::iterator it = get_control_map().find(command);
    if (it == get_control_map().end()) {
        return help();
    } else {
        return (*it).second->call(argc, argv);
    }
}

std::string control_t::help() {
    std::string res;
    for (control_map_t::iterator it = get_control_map().begin(); it != get_control_map().end(); it++) {
        if ((*it).second->help_string.size() == 0) continue;
        res += ((*it).first + ": " + (*it).second->help_string + "\r\n");
    }
    return res;
}

control_t::control_t(const std::string& _key, const std::string& _help_string)
    : key(_key), help_string(_help_string)
{
    rassert(key.size() > 0);
    // TODO: What is this .lock() and .unlock() pair, use RAII.
    get_control_lock().lock();
    rassert(get_control_map().find(key) == get_control_map().end());
    get_control_map()[key] = this;
    get_control_lock().unlock();
}

control_t::~control_t() {
    get_control_lock().lock();
    control_map_t &map = get_control_map();
    control_map_t::iterator it = map.find(key);
    rassert(it != map.end());
    map.erase(it);
    get_control_lock().unlock();
}

/* Example of how to add a control */
struct hi_t : public control_t
{
private:
    int counter;
public:
    hi_t(std::string key)
        : control_t(key, ""), counter(0)
    {}
    std::string call(UNUSED int argc, UNUSED char **argv) {
        counter++;
        if (counter < 3)
            return "Salutations, user.\r\n";
        else if (counter < 4)
            return "Say hi again, I dare you.\r\n";
        else
            return "Base QPS decreased by 100,000.\r\n";
    }
};

hi_t hi("hi");
