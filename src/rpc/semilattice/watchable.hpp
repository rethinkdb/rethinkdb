// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef RPC_SEMILATTICE_WATCHABLE_HPP_
#define RPC_SEMILATTICE_WATCHABLE_HPP_

template <class T>
class semilattice_watchable_t : public watchable_t<T> {
public:
    semilattice_watchable_t() { }
    explicit semilattice_watchable_t(const boost::shared_ptr<semilattice_read_view_t<T> > &_view)
        : view(_view) { }

    semilattice_watchable_t *clone() const {
        return new semilattice_watchable_t(view);
    }

    void apply_read(const std::function<void(const T*)> &read) {
        // Inefficient but generic implementation that works with any view.
        const T current_value = view->get();
        ASSERT_NO_CORO_WAITING;
        read(&current_value);
    }

    T get() {
        return view->get();
    }

    publisher_t<boost::function<void()> > *get_publisher() {
        return view->get_publisher();
    }

    rwi_lock_assertion_t *get_rwi_lock_assertion() {
        return &rwi_lock_assertion;
    }

private:
    boost::shared_ptr<semilattice_read_view_t<T> > view;
    rwi_lock_assertion_t rwi_lock_assertion;

    DISABLE_COPYING(semilattice_watchable_t);
};

template<class T>
cross_thread_watchable_variable_t<T> cross_thread_watchable_from_semilattice(boost::shared_ptr<semilattice_read_view_t<T> > view,
                                                                             threadnum_t dest_thread) {
    return cross_thread_watchable_variable_t<T>(new semilattice_watchable_t<T>(view), dest_thread);
}

#endif  // RPC_SEMILATTICE_WATCHABLE_HPP_
