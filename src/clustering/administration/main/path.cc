#include "clustering/administration/main/path.hpp"

#include "errors.hpp"
#include <boost/tokenizer.hpp>

static const char *const unix_path_separator = "/";

path_t parse_as_path(const std::string &path) {
    path_t res;
    res.is_absolute = (path[0] == unix_path_separator[0]);

    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

    boost::char_separator<char> sep(unix_path_separator);
    tokenizer tokens(path, sep);

    res.nodes.assign(tokens.begin(), tokens.end());

    return res;
}

std::string render_as_path(const path_t &path) {
    std::string res;
    for (std::vector<std::string>::const_iterator it =  path.nodes.begin();
                                                  it != path.nodes.end();
                                                  ++it) {
        if (it != path.nodes.begin() || path.is_absolute) {
            res += unix_path_separator;
        }
        res += *it;
    }

    return res;
}
