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

std::string control_exec(std::string command_and_args) {
    std::string res;

    /* seperate command and arguments */
    int command_end = command_and_args.find(':');
    std::string command, args;
    logINF("command_end: %d\n", command_end);
    command = command_and_args.substr(0, command_end);
    if (command_end==-1) {
        args = std::string("");
    } else {
        args = command_and_args.substr(command_end + 1);
    }

    /* strip off the leading and trailing spaces */
    while (command[command.length() - 1] == ' ')
        command.erase(command.length() - 1, 1);

    while (args[0] == ' ')
        args.erase(0, 1);

    logINF("command: |%s|\n", command.c_str());
    logINF("args: |%s|\n", args.c_str());

    for (control_map_t::iterator it = get_control_map().find(command); it != get_control_map().end() && (*it).first == command; it++)
        res += (*it).second->call(args);

    return res;
}

std::string control_help() {
    std::string res, last_key, last_help;
    for (control_map_t::iterator it = get_control_map().begin(); it != get_control_map().end(); it++) {
        if (((*it).first != last_key || (*it).second->help != last_help) && (*it).second->help.length() > 0)
            res += ((*it).first + std::string(": ") + (*it).second->help + std::string("\r\n"));

        last_key = (*it).first;
        last_help = (*it).second->help;
    }
    return res;
}

control_t::control_t(std::string key, std::string help) 
    : key(key), help(help)
{
    guarantee(key != ""); //this could potentiall cause some errors with control_help
    //TODO @jdoliner make sure that the command doesn't contain any ':'s
    get_control_lock().lock();
    get_control_map().insert(std::pair<std::string, control_t*>(key, this));
    get_control_lock().unlock();
}

control_t::~control_t() {
    get_control_lock().lock();
    for (control_map_t::iterator it = get_control_map().find(key); it != get_control_map().end(); it++)
        if ((*it).second == this)
            get_control_map().erase(it);
    get_control_lock().unlock();
}

/* Example of how to add a control */
struct hi_t : public control_t
{
private:
    int counter;
public:
    hi_t(std::string key)
        : control_t(key, std::string("")), counter(0)
    {}
    std::string call(std::string) {
        counter++;
        if (counter < 3)
            return std::string("Salutations, user.\n");
        else if (counter < 4)
            return std::string("Say hi again, I dare you.\n");
        else
            return std::string("Base QPS decreased by 100,000.\n");
    }
};

hi_t hi(std::string("hi"));
