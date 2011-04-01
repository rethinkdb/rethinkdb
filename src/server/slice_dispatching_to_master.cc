#include "replication/master.hpp"

#include "server/slice_dispatching_to_master.hpp"

using replication::master_t;

master_dispatcher_t::master_dispatcher_t(int slice_home_thread, master_t *master)
    : slice_home_thread_(slice_home_thread), master_(master) { }

struct change_visitor_t : public boost::static_visitor<mutation_t> {
    master_t *master;
    castime_t castime;
    mutation_t operator()(const get_cas_mutation_t& m) {
        coro_t::spawn_on_thread(master->home_thread, boost::bind(&master_t::get_cas, master, m.key, castime));
        return m;
    }
    mutation_t operator()(const sarc_mutation_t& m) {
        unique_ptr_t<buffer_borrowing_data_provider_t> borrower(new buffer_borrowing_data_provider_t(master->home_thread, m.data));
        coro_t::spawn_on_thread(master->home_thread, boost::bind(&master_t::sarc, master,
            m.key, borrower->side_provider(), m.flags, m.exptime, castime, m.add_policy, m.replace_policy, m.old_cas));
        sarc_mutation_t m2(m);
        m2.data = borrower;
        return m2;
    }
    mutation_t operator()(const incr_decr_mutation_t& m) {
        coro_t::spawn_on_thread(master->home_thread, boost::bind(&master_t::incr_decr, master, m.kind, m.key, m.amount, castime));
        return m;
    }
    mutation_t operator()(const append_prepend_mutation_t &m) {
        unique_ptr_t<buffer_borrowing_data_provider_t> borrower(new buffer_borrowing_data_provider_t(master->home_thread, m.data));
        coro_t::spawn_on_thread(master->home_thread, boost::bind(&master_t::append_prepend, master,
            m.kind, m.key, borrower->side_provider(), castime));
        append_prepend_mutation_t m2(m);
        m2.data = borrower;
        return m2;
    }
    mutation_t operator()(const delete_mutation_t& m) {
        coro_t::spawn_on_thread(master->home_thread, boost::bind(&master_t::delete_key, master, m.key, castime.timestamp));
        return m;
    }
};


mutation_t master_dispatcher_t::dispatch_change(const mutation_t& m, castime_t castime) {
    rassert(get_thread_id() == slice_home_thread_);
    if (master_ != NULL) {
        change_visitor_t functor;
        functor.master = master_;
        functor.castime = castime;
        return boost::apply_visitor(functor, m.mutation);
    } else {
        return m;
    }
}









btree_slice_dispatching_to_master_t::btree_slice_dispatching_to_master_t(btree_slice_t *slice, snag_ptr_t<replication::master_t>& master) : slice_(slice), master_(master) {
    if (master_.get() != NULL) {
        master_->register_dispatcher(this);
    }
}



mutation_result_t btree_slice_dispatching_to_master_t::change(const mutation_t& m, castime_t castime) {
    // TODO: Get rid of this function entirely, of course.
    on_thread_t th(slice_->home_thread);
    return slice_->change(m, castime);
}

void btree_slice_dispatching_to_master_t::nop_back_on_masters_thread(repli_timestamp timestamp, cond_t *cond, int *counter) {
    rassert(get_thread_id() == master_->home_thread);

    repli_timestamp t;
    {
        on_thread_t th(slice_->home_thread);

        t = current_time();
        // TODO: Don't crash just because the slave sent a bunch of crap to us.  Just disconnect the slave.
        guarantee(t.time >= timestamp.time);
    }

    --*counter;
    rassert(*counter >= 0);
    if (*counter == 0) {
        cond->pulse();
    }
}

void btree_slice_dispatching_to_master_t::spawn_backfill(repli_timestamp since_when, backfill_callback_t *callback) {
    callback->add_dual_backfiller_hold();
    coro_t::spawn_on_thread(slice_->home_thread, boost::bind(&btree_slice_t::backfill, slice_, since_when, callback));
}
