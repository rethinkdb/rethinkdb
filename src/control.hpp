#ifndef __CONTROL_HPP__
#define __CONTROL_HPP__

#include <string>
#include <map>

std::string control_exec(std::string);

std::string control_help();

class control_t
{
public:
    control_t(std::string, std::string);
    ~control_t();

private:
    std::string key;

public:
    virtual std::string call(std::string) = 0;

private:
    std::string help;

friend std::string control_help();
};

typedef  std::map<std::string, control_t*> control_map_t;

#endif /*__CONTROL_HPP__*/
