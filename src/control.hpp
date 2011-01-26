#ifndef __CONTROL_HPP__
#define __CONTROL_HPP__

#include <string>
#include <map>

class control_t
{
public:
    control_t(std::string);
    ~control_t();
private:
    std::string key;
public:
    virtual std::string call() = 0;
};

typedef  std::map<std::string, control_t*> control_map_t;

std::string control_exec(std::string);

#endif /*__CONTROL_HPP__*/
