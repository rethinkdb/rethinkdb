#include "server/control.hpp"

#include "logger.hpp"
#include "errors.hpp"
#include "concurrency/multi_wait.hpp"

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

control_t::control_t(const std::string& _key, const std::string& _help_string, bool _secret)
    : key(_key), help_string(_help_string), secret(_secret)
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
struct help_control_t : public control_t
{
    help_control_t() : control_t("help", "Display this help message.") { }
    std::string call(int argc, char **argv) {

        bool show_secret = false;
        if (argc == 2 && argv[1] == std::string("secret")) {
            show_secret = true;
        } else if (argc != 1) {
            return "\"rethinkdb help\" does not expect a parameter.\r\n";
        }

#ifndef NDEBUG
        show_secret = true;
#endif

        spinlock_acq_t control_acq(&get_control_lock());
        std::string res = "\r\nThe following commands are available:\r\n\r\n";
        for (control_map_t::iterator it = get_control_map().begin(); it != get_control_map().end(); it++) {
            if ((*it).second->secret && !show_secret) continue;
            res += (*it).first + ": " + (*it).second->help_string;
            if ((*it).second->secret) res += " (secret)";
            res += "\r\n";
        }
        res += "\r\n";
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
