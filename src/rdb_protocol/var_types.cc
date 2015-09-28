// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/var_types.hpp"

#include "containers/archive/stl_types.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/env.hpp"
#include "stl_utils.hpp"

namespace ql {

var_visibility_t::var_visibility_t() : implicit_depth(0) { }

var_visibility_t var_visibility_t::with_func_arg_name_list(const std::vector<sym_t> &arg_names) const {
    var_visibility_t ret = *this;
    ret.visibles.insert(arg_names.begin(), arg_names.end());
    if (function_emits_implicit_variable(arg_names)) {
        ++ret.implicit_depth;
    }
    return ret;
}

bool var_visibility_t::contains_var(sym_t varname) const {
    return visibles.find(varname) != visibles.end();
}

bool var_visibility_t::implicit_is_accessible() const {
    return implicit_depth == 1;
}

void debug_print(printf_buffer_t *buf, const var_visibility_t &var_visibility) {
    buf->appendf("var_visibility{implicit_depth=%" PRIu32 ", visibles=", var_visibility.implicit_depth);
    debug_print(buf, var_visibility.visibles);
    buf->appendf("}");
}

var_captures_t::var_captures_t(var_captures_t &&movee)
    : vars_captured(std::move(movee.vars_captured)),
      implicit_is_captured(std::move(movee.implicit_is_captured)) {
    // Just to leave things in a clean state...
    movee.implicit_is_captured = false;
}

var_captures_t &var_captures_t::operator=(var_captures_t &&movee) {
    var_captures_t tmp(std::move(movee));
    vars_captured.swap(tmp.vars_captured);
    implicit_is_captured = tmp.implicit_is_captured;
    return *this;
}


var_scope_t::var_scope_t() : implicit_depth(0) { }

var_scope_t var_scope_t::with_func_arg_list(
    const std::vector<sym_t> &arg_names,
    const std::vector<datum_t> &arg_values) const {
    r_sanity_check(arg_names.size() == arg_values.size());
    var_scope_t ret = *this;
    if (function_emits_implicit_variable(arg_names)) {
        if (ret.implicit_depth == 0) {
            ret.maybe_implicit = arg_values[0];
        } else {
            ret.maybe_implicit.reset();
        }
        ++ret.implicit_depth;
    }

    for (size_t i = 0; i < arg_names.size(); ++i) {
        r_sanity_check(arg_values[i].has());
        ret.vars.insert(std::make_pair(arg_names[i], arg_values[i]));
    }
    return ret;
}

var_scope_t var_scope_t::filtered_by_captures(const var_captures_t &captures) const {
    var_scope_t ret;
    for (auto it = captures.vars_captured.begin(); it != captures.vars_captured.end(); ++it) {
        auto vars_it = vars.find(*it);
        r_sanity_check(vars_it != vars.end());
        ret.vars.insert(*vars_it);
    }
    ret.implicit_depth = implicit_depth;
    if (captures.implicit_is_captured) {
        r_sanity_check(implicit_depth == 1);
        ret.maybe_implicit = maybe_implicit;
    }
    return ret;
}

datum_t var_scope_t::lookup_var(sym_t varname) const {
    auto it = vars.find(varname);
    // This is a sanity check because we should never have constructed an expression
    // with an invalid variable name.
    r_sanity_check(it != vars.end());
    return it->second;
}

datum_t var_scope_t::lookup_implicit() const {
    r_sanity_check(implicit_depth == 1 && maybe_implicit.has());
    return maybe_implicit;
}

std::string var_scope_t::print() const {
    std::string ret = "[";
    if (implicit_depth == 0) {
        ret += "(no implicit)";
    } else if (implicit_depth == 1) {
        ret += "implicit: ";
        if (maybe_implicit.has()) {
            ret += maybe_implicit.print();
        } else {
            ret += "(not stored)";
        }
    } else {
        ret += "(multiple implicits)";
    }

    for (auto it = vars.begin(); it != vars.end(); ++it) {
        ret += ", ";
        ret += strprintf("%" PRIi64 ": ", it->first.value);
        ret += it->second.print();
    }
    ret += "]";
    return ret;
}

var_visibility_t var_scope_t::compute_visibility() const {
    var_visibility_t ret;
    for (auto it = vars.begin(); it != vars.end(); ++it) {
        ret.visibles.insert(it->first);
    }
    ret.implicit_depth = implicit_depth;
    return ret;
}

template <cluster_version_t W>
void serialize(write_message_t *wm, const var_scope_t &vs) {
    serialize<W>(wm, vs.vars);
    serialize<W>(wm, vs.implicit_depth);
    if (vs.implicit_depth == 1) {
        const bool has = vs.maybe_implicit.has();
        serialize<W>(wm, has);
        if (has) {
            serialize<W>(wm, vs.maybe_implicit);
        }
    }
}

template <cluster_version_t W>
archive_result_t deserialize(read_stream_t *s, var_scope_t *vs) {
    std::map<sym_t, datum_t> local_vars;
    archive_result_t res = deserialize<W>(s, &local_vars);
    if (bad(res)) { return res; }

    uint32_t local_implicit_depth;
    res = deserialize<W>(s, &local_implicit_depth);
    if (bad(res)) { return res; }

    datum_t local_maybe_implicit;
    if (local_implicit_depth == 1) {
        bool has;
        res = deserialize<W>(s, &has);
        if (bad(res)) { return res; }

        if (has) {
            res = deserialize<W>(s, &local_maybe_implicit);
            if (bad(res)) { return res; }
        }
    }

    vs->vars = std::move(local_vars);
    vs->implicit_depth = local_implicit_depth;
    vs->maybe_implicit = std::move(local_maybe_implicit);
    return archive_result_t::SUCCESS;
}

INSTANTIATE_SERIALIZE_FOR_CLUSTER_AND_DISK(var_scope_t);

template archive_result_t
deserialize<cluster_version_t::v1_14>(read_stream_t *s, var_scope_t *);
template archive_result_t
deserialize<cluster_version_t::v1_15>(read_stream_t *s, var_scope_t *);
template archive_result_t
deserialize<cluster_version_t::v1_16>(read_stream_t *s, var_scope_t *);
template archive_result_t
deserialize<cluster_version_t::v2_0>(read_stream_t *s, var_scope_t *);
template archive_result_t
deserialize<cluster_version_t::v2_1>(read_stream_t *s, var_scope_t *);
template archive_result_t
deserialize<cluster_version_t::v2_2_is_latest>(read_stream_t *s, var_scope_t *);

}  // namespace ql
