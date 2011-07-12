#include "server/control.hpp"

#include <map>

#include "arch/spinlock.hpp"

#include "utils.hpp"
#include "logger.hpp"
#include "errors.hpp"

typedef std::map<std::string, control_t *> control_map_t;

control_map_t& get_control_map() {
    /* Getter function so that we can be sure that control_list is initialized before it is needed,
    as advised by the C++ FAQ. Otherwise, a control_t might be initialized before the control list
    was initialized. */
    
    static control_map_t control_map;
    return control_map;
}

spinlock_t& get_control_lock() {
    /* To avoid static initialization fiasco */
    
    static spinlock_t lock;
    return lock;
}

std::string control_t::exec(int argc, char **argv) {
    if (argc == 0) {
        return "You must provide a command name. Type \"rethinkdb help\" for a list of available "
            "commands.\r\n";
    }
    std::string command = argv[0];

    control_map_t::iterator it = get_control_map().find(command);
    if (it == get_control_map().end()) {
        return "There is no command called \"" + command + "\". Type \"rethinkdb help\" for a "
            "list of available commands.\r\n";
    } else {
        return (*it).second->call(argc, argv);
    }
}

control_t::control_t(const std::string& _key, const std::string& _help_string, bool _internal)
    : key(_key), help_string(_help_string), internal(_internal)
{
    rassert(key.size() > 0);
    spinlock_acq_t control_acq(&get_control_lock());
    rassert(get_control_map().find(key) == get_control_map().end());
    get_control_map()[key] = this;
}

control_t::~control_t() {
    spinlock_acq_t control_acq(&get_control_lock());
    control_map_t &map = get_control_map();
    control_map_t::iterator it = map.find(key);
    rassert(it != map.end());
    map.erase(it);
}

/* Control that displays a list of controls */
class help_control_t : public control_t
{
public:
    help_control_t() : control_t("help", "Display this help message (use \"rethinkdb help internal\" to show internal commands).") { }
    std::string call(int argc, char **argv) {
        bool show_internal = argc == 2 && argv[1] == std::string("internal");

        spinlock_acq_t control_acq(&get_control_lock());
        std::string res = strprintf("The following %scommands are available:\r\n", show_internal ? "internal " : "");
        for (control_map_t::iterator it = get_control_map().begin(); it != get_control_map().end(); it++) {
            if (it->second->internal != show_internal) continue;
            res += "rethinkdb " + it->first + ": " + it->second->help_string + "\r\n";
        }
        return res;
    }
} help_control;

/* Example of how to add a control */
struct hi_control_t : public control_t
{
private:
    int counter;
public:
    hi_control_t() :
        control_t("hi", "Say 'hi' to the server, and it will say 'hi' back.", true),
        counter(0)
        { }
    std::string call(UNUSED int argc, UNUSED char **argv) {
        counter++;
        if (counter < 3)
            return "Salutations, user.\r\n";
        else if (counter < 4)
            return "Say hi again, I dare you.\r\n";
        else
            return "Base QPS decreased by 100,000.\r\n";
    }
} hi_control;
