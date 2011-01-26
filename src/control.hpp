#ifndef __CONTROL_HPP__
#define __CONTROL_HPP__

#include <string>
#include <map>

class control_t
{
public:
    control_t(std::string, std::string);
    ~control_t();

private:
    std::string key;

public:
    virtual std::string call() = 0;

public:
    std::string help;
};

typedef  std::map<std::string, control_t*> control_map_t;

std::string control_exec(std::string);

std::string control_help();

#endif /*__CONTROL_HPP__*/
