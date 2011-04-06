#ifndef __SERVER_CONTROL_HPP__
#define __SERVER_CONTROL_HPP__

#include <string>
#include <map>

class control_t {
public:
    control_t(const std::string& key, const std::string& help_string);
    virtual ~control_t();

    virtual std::string call(int argc, char **argv) = 0;

    static std::string exec(int argc, char **argv);
    static std::string help();


private:
    std::string key;
    std::string help_string;
};

typedef std::map<std::string, control_t *> control_map_t;

#endif  // __SERVER_CONTROL_HPP__
