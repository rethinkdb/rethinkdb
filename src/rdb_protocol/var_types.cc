// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/var_types.hpp"

#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/env.hpp"
#include "stl_utils.hpp"

namespace ql {

var_visibility_t::var_visibility_t() : implicit_depth(0) { }

var_visibility_t var_visibility_t::with_func_arg_name_list(const std::vector<sym_t> &new_visibles) const {
    var_visibility_t ret = *this;
    // RSI: Maybe we should check for overlap and fail (because each function's symbol in the syntax tree should be different).
    ret.visibles.insert(new_visibles.begin(), new_visibles.end());
    if (new_visibles.size() == 1 && gensym_t::var_allows_implicit(new_visibles[0])) {
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

var_scope_t::var_scope_t() : implicit_depth(0) { }

var_scope_t var_scope_t::with_func_arg_list(const std::vector<std::pair<sym_t, counted_t<const datum_t> > > &new_vars) const {
    var_scope_t ret = *this;
    if (new_vars.size() == 1 && gensym_t::var_allows_implicit(new_vars[0].first)) {
        if (ret.implicit_depth == 0) {
            ret.maybe_implicit = new_vars[0].second;
        } else {
            ret.maybe_implicit.reset();
        }
        ++ret.implicit_depth;
    }

    ret.vars.insert(new_vars.begin(), new_vars.end());
    return ret;
}

counted_t<const datum_t> var_scope_t::lookup_var(sym_t varname) const {
    auto it = vars.find(varname);
    // This is a sanity check because we should never have constructed an expression
    // with an invalid variable name.
    r_sanity_check(it != vars.end());
    return it->second;
}

counted_t<const datum_t> var_scope_t::lookup_implicit() const {
    r_sanity_check(implicit_depth == 1 && maybe_implicit.has());
    return maybe_implicit;
}

std::string var_scope_t::print() const {
    std::string ret = "[";
    if (implicit_depth == 0) {
        ret += "(no implicit)";
    } else if (implicit_depth == 1) {
        ret += "implicit: ";
        r_sanity_check(maybe_implicit.has());
        ret += maybe_implicit->print();
    } else {
        ret += "(multiple implicits)";
    }

    for (auto it = vars.begin(); it != vars.end(); ++it) {
        ret += ", ";
        ret += strprintf("%" PRIi64 ": ", it->first.value);
        ret += it->second->print();
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

void var_scope_t::rdb_serialize(write_message_t &msg) const {  // NOLINT(runtime/references)
    msg << vars;
    msg << implicit_depth;
    if (implicit_depth == 1) {
        msg << maybe_implicit;
    }
}

archive_result_t var_scope_t::rdb_deserialize(read_stream_t *s) {
    std::map<sym_t, counted_t<const datum_t> > local_vars;
    archive_result_t res = deserialize(s, &local_vars);
    if (res) { return res; }
    uint32_t local_implicit_depth;
    res = deserialize(s, &local_implicit_depth);
    if (res) { return res; }
    counted_t<const datum_t> local_maybe_implicit;
    if (local_implicit_depth == 1) {
        res = deserialize(s, &local_maybe_implicit);
        if (res) { return res; }
    }

    vars = std::move(local_vars);
    implicit_depth = local_implicit_depth;
    maybe_implicit = std::move(local_maybe_implicit);
    return ARCHIVE_SUCCESS;
}


}  // namespace ql
