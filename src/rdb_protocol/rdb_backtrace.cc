// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "rdb_protocol/rdb_backtrace.hpp"

#include <algorithm>

namespace ql {

const datum_t backtrace_registry_t::EMPTY_BACKTRACE = datum_t::empty_array();

backtrace_registry_t::frame_t::frame_t(backtrace_id_t _parent, datum_t _val) :
    parent(_parent), val(_val) { }

bool backtrace_registry_t::frame_t::is_head() const {
    return val.get_type() == datum_t::type_t::R_NULL;
}

backtrace_registry_t::backtrace_registry_t() {
    frames.emplace_back(backtrace_id_t::empty(), datum_t::null());
}

backtrace_id_t backtrace_registry_t::new_frame(backtrace_id_t parent_bt,
                                               const datum_t &val) {
    frames.emplace_back(parent_bt, val);
    return backtrace_id_t(frames.size() - 1);
}

datum_t backtrace_registry_t::datum_backtrace(const exc_t &ex) const {
    return datum_backtrace(ex.backtrace(), ex.dummy_frames());
}

datum_t backtrace_registry_t::datum_backtrace(backtrace_id_t bt,
                                              size_t dummy_frames) const {
    r_sanity_check(bt.get() < frames.size());
    std::vector<datum_t> res;
    for (const frame_t *f = &frames[bt.get()];
         !f->is_head(); f = &frames[f->parent.get()]) {
        r_sanity_check(f->parent.get() < frames.size());
        if (dummy_frames > 0) {
            --dummy_frames;
        } else {
            res.push_back(f->val);
        }
    }
    std::reverse(res.begin(), res.end());
    return datum_t(std::move(res), configured_limits_t::unlimited);
}

} // namespace ql
