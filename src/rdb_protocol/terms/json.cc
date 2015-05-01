// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "cjson/json.hpp"
#include "rdb_protocol/op.hpp"
#include "rdb_protocol/term.hpp"
#include "rdb_protocol/terms/terms.hpp"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace ql {
class json_term_t : public op_term_t {
public:
    json_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1)) { }

    scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        const datum_string_t data = args->arg(env, 0)->as_str();

        if (env->env->reql_version() < reql_version_t::v2_1) {
            const std::string std_data = data.to_std();
            scoped_cJSON_t cjson(cJSON_Parse(std_data.c_str()));
            rcheck(cjson.get() != NULL, base_exc_t::GENERIC,
                   strprintf("Failed to parse \"%s\" as JSON.",
                     (data.size() > 40
                      ? (std_data.substr(0, 37) + "...").c_str()
                      : std_data.c_str())));
            return new_val(to_datum(cjson.get(), env->env->limits(),
                                    env->env->reql_version()));
        } else {
            // Copy the string into a null-terminated c-string that we can write to,
            // so we can use RapidJSON in-situ parsing (and at least avoid some additional
            // copying).
            std::vector<char> str_buf(data.size() + 1);
            memcpy(str_buf.data(), data.data(), data.size());
            for (size_t i = 0; i < data.size(); ++i) {
                rcheck(str_buf[i] != '\0', base_exc_t::GENERIC,
                       "Encountered unescaped null byte in JSON string.");
            }
            str_buf[data.size()] = '\0';

            rapidjson::Document json;
            // Note: Insitu will cause some parts of `json` to directly point into
            // `str_buf`. `str_buf`'s life time must be at least as long as `json`'s.
            json.ParseInsitu(str_buf.data());

            rcheck(!json.HasParseError(), base_exc_t::GENERIC,
                   strprintf("Failed to parse \"%s\" as JSON: %s",
                       (data.size() > 40
                        ? (data.to_std().substr(0, 37) + "...").c_str()
                        : data.to_std().c_str()),
                       rapidjson::GetParseError_En(json.GetParseError())));
            return new_val(to_datum(json, env->env->limits(),
                                    env->env->reql_version()));
        }
    }

    virtual const char *name() const { return "json"; }
};

class to_json_string_term_t : public op_term_t {
public:
    to_json_string_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1)) { }

    scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        scoped_ptr_t<val_t> v = args->arg(env, 0);
        datum_t d = v->as_datum();
        r_sanity_check(d.has());
        if (env->env->reql_version() < reql_version_t::v2_1) {
            scoped_cJSON_t json = d.as_json();
            return new_val(datum_t(datum_string_t(json.PrintUnformatted())));
        } else {
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            d.write_json(&writer);
            return new_val(datum_t(datum_string_t(buffer.GetSize(), buffer.GetString())));
        }
    }

    virtual const char *name() const { return "to_json_string"; }
};

counted_t<term_t> make_json_term(
        compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<json_term_t>(env, term);
}

counted_t<term_t> make_to_json_string_term(
        compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<to_json_string_term_t>(env, term);
}
} // namespace ql
