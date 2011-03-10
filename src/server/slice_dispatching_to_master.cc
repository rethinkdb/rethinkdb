#include "server/slice_dispatching_to_master.hpp"

#include "replication/master.hpp"

using replication::master_t;

/* set_store_t interface. */

struct change_visitor_t : public boost::static_visitor<mutation_result_t> {
    master_t *master;
    btree_slice_t *slice;
    castime_t castime;
    mutation_result_t operator()(const get_cas_mutation_t &m) {
        spawn_on_home(master, boost::bind(&master_t::get_cas, _1, m.key, castime));
        return slice->change(m, castime);
    }
    mutation_result_t operator()(const set_mutation_t &m) {
        buffer_borrowing_data_provider_t borrower(master->home_thread, m.data);
        spawn_on_home(master, boost::bind(&master_t::sarc, _1,
            m.key, borrower.side_provider(), m.flags, m.exptime, castime, m.add_policy, m.replace_policy, m.old_cas));
        set_mutation_t m2(m);
        m2.data = &borrower;
        return slice->change(m2, castime);
    }
    mutation_result_t operator()(const incr_decr_mutation_t &m) {
        spawn_on_home(master, boost::bind(&master_t::incr_decr, _1, m.kind, m.key, m.amount, castime));
        return slice->change(m, castime);
    }
    mutation_result_t operator()(const append_prepend_mutation_t &m) {
        buffer_borrowing_data_provider_t borrower(master->home_thread, m.data);
        spawn_on_home(master, boost::bind(&master_t::append_prepend, _1,
            m.kind, m.key, borrower.side_provider(), castime));
        append_prepend_mutation_t m2(m);
        m2.data = &borrower;
        return slice->change(m2, castime);
    }
    mutation_result_t operator()(const delete_mutation_t &m) {
        spawn_on_home(master, boost::bind(&master_t::delete_key, _1, m.key, castime.timestamp));
        return slice->change(m, castime);
    }
};

mutation_result_t btree_slice_dispatching_to_master_t::change(const mutation_t &m, castime_t castime) {

    on_thread_t th(slice_->home_thread);
    if (master_) {
        change_visitor_t functor;
        functor.master = master_;
        functor.slice = slice_;
        functor.castime = castime;
        return boost::apply_visitor(functor, m.mutation);
    } else {
        return slice_->change(m, castime);
    }
}
