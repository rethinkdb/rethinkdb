#ifndef __CONTROL_HPP__
#define __CONTROL_HPP__

#include <string>
#include <map>

using std::string;

class control_t {
public:
    control_t(string, string);
    ~control_t();

    virtual string call(int argc, char **argv) = 0;

    static string exec(int argc, char **argv);
    static string help();


private:
    string key;
    string help_string;
};

typedef std::map<string, control_t*> control_map_t;

#endif /*__CONTROL_HPP__*/
