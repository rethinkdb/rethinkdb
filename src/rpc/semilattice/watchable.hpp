#ifndef __RPC_SEMILATTICE_WATCHABLE_HPP__
#define __RPC_SEMILATTICE_WATCHABLE_HPP__

template <class T>
class semilattice_watchable_t : public watchable_t<T> {
public:
    semilattice_watchable_t() { }
    explicit semilattice_watchable_t(const boost::shared_ptr<semilattice_read_view_t<T> > &_view)
        : view(_view) { }

    semilattice_watchable_t *clone() const {
        return new semilattice_watchable_t(view);
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
cross_thread_watchable_variable_t<T> cross_thread_watchable_from_semilattice(boost::shared_ptr<semilattice_read_view_t<T> > view, int dest_thread) {
    return cross_thread_watchable_variable_t<T>(new semilattice_watchable_t<T>(view), dest_thread);
}

#endif
