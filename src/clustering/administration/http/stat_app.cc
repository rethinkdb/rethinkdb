#include "errors.hpp"
#include <boost/tokenizer.hpp>
#include <string>
#include <set>
#include "clustering/administration/http/stat_app.hpp"
#include "perfmon.hpp"
#include "http/json.hpp"

static boost::char_separator<char> stat_name_sep(",", "", boost::drop_empty_tokens);

stat_http_app_t::stat_http_app_t() {
}

http_res_t stat_http_app_t::handle(const http_req_t &req) {
    scoped_cJSON_t body(cJSON_CreateObject());

    perfmon_stats_t stats;
    perfmon_get_stats(&stats, true);
    
    boost::optional<std::string> stat_names_q = req.find_query_param("q");
    boost::optional<std::set<std::string> > stat_names;
    if (!stat_names_q) {
        // THIS SPACE WAS INTENTIONALLY LEFT BLANK: return all stats, stat_names map should be boost::none
    } else {
        // return only the requested stats
        typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
        typedef tokenizer::iterator stat_name_iterator;

        tokenizer t(stat_names_q.get(), stat_name_sep);
        stat_names.reset(std::set<std::string>());
        for (stat_name_iterator it = t.begin(); it != t.end(); ++it) {
            stat_names.get().insert(*it);
        }
    }
    for (perfmon_stats_t::iterator it = stats.begin(); it != stats.end(); ++it) {
        if (!(!stat_names) && stat_names->find(it->first) == stat_names->end()) {
            continue;
        } else {
            cJSON_AddItemToObject(body.get(), it->first.c_str(), cJSON_CreateString(it->second.c_str()));
        }
    }

    http_res_t res(200);
    res.set_body("application/json", cJSON_print_std_string(body.get()));
    return res;
}
