#ifndef __CONTROL_HPP__
#define __CONTROL_HPP__

#include <string>
#include <map>

using namespace std;

string control_exec(string);

string control_help();

class control_t
{
public:
    control_t(string, string);
    ~control_t();

private:
    string key;

public:
    virtual string call(string) = 0;

private:
    string help;

friend string control_help();
};

typedef  map<string, control_t*> control_map_t;

#endif /*__CONTROL_HPP__*/
