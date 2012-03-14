#ifndef __RPC_DIRECTORY_WATCHABLE_COPIER_HPP__
#define __RPC_DIRECTORY_WATCHABLE_COPIER_HPP__

#include "concurrency/auto_drainer.hpp"
#include "concurrency/watchable.hpp"
#include "rpc/directory/view.hpp"

/* Eventually we'd like to use `watchable_t` in place of all of the directory
views. `watchable_write_copier_t` facilitates the conversion from
`directory_wview_t` to `watchable_t`. It takes a `watchable_t<T>` and a
`directory_wview_t<T>` and it copies updates from the one to the other.
`translate_into_watchable()` facilitates the conversion from `directory_rview_t`
to `watchable_t`; it takes a `directory_rview_t<T>` and returns a
`watchable_t<std::map<peer_id_t, T> >`. */

template<class metadata_t>
class watchable_write_copier_t {
public:
    watchable_write_copier_t(
            const clone_ptr_t<watchable_t<metadata_t> > &s,
            const clone_ptr_t<directory_wview_t<metadata_t> > &d) :
        source(s), dest(d), subscription(boost::bind(&watchable_write_copier_t::spawn_update, this))
    {
        typename watchable_t<metadata_t>::freeze_t freeze(source);
        spawn_update();
        subscription.reset(source, &freeze);
    }

private:
    void spawn_update() {
        coro_t::spawn_sometime(boost::bind(
            &watchable_write_copier_t<metadata_t>::update, this,
            source->get(), auto_drainer_t::lock_t(&drainer)
            ));
    }

    void update(const metadata_t &new_value, auto_drainer_t::lock_t) {
        directory_write_service_t::our_value_lock_acq_t lock(dest->get_directory_service());
        dest->set_our_value(new_value, &lock);
    }

    clone_ptr_t<watchable_t<metadata_t> > source;
    clone_ptr_t<directory_wview_t<metadata_t> > dest;
    auto_drainer_t drainer;
    typename watchable_t<metadata_t>::subscription_t subscription;
};

template<class metadata_t>
clone_ptr_t<watchable_t<std::map<peer_id_t, metadata_t> > > translate_into_watchable(
    const clone_ptr_t<directory_rview_t<metadata_t> > &view);

/* C++ can't cast `clone_ptr_t<directory_rwview_t<T> >` to
`clone_ptr_t<directory_rview_t<T> >` */
template<class metadata_t>
clone_ptr_t<watchable_t<std::map<peer_id_t, metadata_t> > > translate_into_watchable(
    const clone_ptr_t<directory_rwview_t<metadata_t> > &view);

#include "watchable_copier.tcc"

#endif /* __RPC_DIRECTORY_WATCHABLE_COPIER_HPP__ */
