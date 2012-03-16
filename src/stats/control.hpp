#ifndef STATS_CONTROL_HPP_
#define STATS_CONTROL_HPP_

/* This isn't really part of the stats code. It's in the `stats` subdirectory
because it's like stats in that it's a relatively slow method of bypassing the
normal access channels for administrative purposes. We should rename `stats` to
`admin` or something.

`control.hpp` used to live in the `server/` directory. Tim moved it to the
`stats/` directory because `server/` was going away in the clustering branch but
`control.hpp` was staying. */

#include <string>

class control_t {
public:
    control_t(const std::string& key, const std::string& help_string, bool internal = false);
    virtual ~control_t();

    virtual std::string call(int argc, char **argv) = 0;

    static std::string exec(int argc, char **argv);

private:
    friend class help_control_t;
    std::string key;
    std::string help_string;
    bool internal;
};

#endif  // STATS_CONTROL_HPP_
