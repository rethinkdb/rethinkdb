// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_VAR_TYPES_HPP_
#define RDB_PROTOCOL_VAR_TYPES_HPP_

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "containers/counted.hpp"
#include "containers/archive/archive.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/sym.hpp"

namespace ql {

class var_visibility_t {
public:
    var_visibility_t();

    // Updates the implicit variable visibility according to the rules.
    var_visibility_t with_func_arg_name_list(const std::vector<sym_t> &arg_names) const;

    bool contains_var(sym_t varname) const;
    bool implicit_is_accessible() const;

    uint32_t get_implicit_depth() const { return implicit_depth; }

private:
    friend class var_scope_t;
    friend void debug_print(printf_buffer_t *buf, const var_visibility_t &var_visibility);


    std::set<sym_t> visibles;
    uint32_t implicit_depth;
};

void debug_print(printf_buffer_t *buf, const var_visibility_t &var_visibility);

class var_captures_t {
public:
    var_captures_t() : implicit_is_captured(false) { }
    var_captures_t(var_captures_t &&);
    var_captures_t &operator=(var_captures_t &&);

    std::set<sym_t> vars_captured;
    bool implicit_is_captured;

    DISABLE_COPYING(var_captures_t);
};

class var_scope_t {
public:
    var_scope_t();

    var_scope_t with_func_arg_list(
        const std::vector<sym_t> &arg_names,
        const std::vector<datum_t> &arg_values) const;

    var_scope_t filtered_by_captures(const var_captures_t &captures) const;

    datum_t lookup_var(sym_t varname) const;
    datum_t lookup_implicit() const;

    // Dumps a complete "human readable" description of the var_scope_t.  Is used by
    // info_term_t via reql_func_t::print_source().
    std::string print() const;

    var_visibility_t compute_visibility() const;

    template <cluster_version_t W>
    void serialize(write_message_t *wm, const var_scope_t &);
    template <cluster_version_t W>
    archive_result_t deserialize(read_stream_t *s, var_scope_t *);

private:
    std::map<sym_t, datum_t> vars;

    uint32_t implicit_depth;

    // If non-empty, implicit_depth == 1.  Might be empty (when implicit_depth == 1),
    // if the value got filtered out (where the body of a func doesn't use it).
    datum_t maybe_implicit;
};

}  // namespace ql



#endif  // RDB_PROTOCOL_VAR_TYPES_HPP_
