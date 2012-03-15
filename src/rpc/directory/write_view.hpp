#ifndef RPC_DIRECTORY_WRITE_VIEW_HPP_
#define RPC_DIRECTORY_WRITE_VIEW_HPP_

#include "concurrency/mutex.hpp"
#include "containers/clone_ptr.hpp"
#include "lens.hpp"

class directory_write_service_t :
    public home_thread_mixin_t
{
public:
    /* `our_value_lock_acq_t` prevents anyone from accessing our directory-value
    other than the person who holds the `our_value_lock_acq_t`. Its constructor
    may block. It can only be acquired on the `directory_write_service_t`'s home
    thread. Note that it only applies to `directory_wview_t`s; it doesn't
    interfere with accessing our value via the `directory_rview_t`'s APIs. */
    class our_value_lock_acq_t {
    public:
        explicit our_value_lock_acq_t(directory_write_service_t *) THROWS_NOTHING;
        void assert_is_holding(directory_write_service_t *) THROWS_NOTHING;
    private:
        mutex_t::acq_t acq;
    };

protected:
    virtual ~directory_write_service_t() { }

private:
    /* Returns a lock for writing to our value. `get_our_value_lock()` should
    only be accessed on the `directory_write_service_t`'s home thread. */
    virtual mutex_t *get_our_value_lock() THROWS_NOTHING = 0;
};

template<class metadata_t>
class directory_wview_t {
public:
    virtual ~directory_wview_t() { }
    virtual directory_wview_t *clone() const = 0;

    /* Returns the current value of this part of the directory for us. This is
    different from `directory_rview_t::get_value()` in that the return value
    from `get_our_value()` is guaranteed to reflect the latest changes made via
    `set_our_value()`, whereas `directory_rview_t::get_value()` is not. */
    virtual metadata_t get_our_value(directory_write_service_t::our_value_lock_acq_t *proof) THROWS_NOTHING = 0;

    /* Changes the value of this part of the directory for the peer that is us.
    You must hold a `directory_write_service_t::our_value_lock_acq_t`; this is
    to avoid race conditions with two different things changing the directory
    metadata simultaneously. `set_our_value()` may block. It can only be called
    on the home thread of the `directory_write_service_t`. */
    virtual void set_our_value(const metadata_t &new_value_for_us, directory_write_service_t::our_value_lock_acq_t *proof) THROWS_NOTHING = 0;

    virtual directory_write_service_t *get_directory_service() THROWS_NOTHING = 0;

    template<class inner_t>
    clone_ptr_t<directory_wview_t<inner_t> > subview(const clone_ptr_t<readwrite_lens_t<inner_t, metadata_t> > &lens);
};

#include "rpc/directory/write_view.tcc"

#endif /* RPC_DIRECTORY_WRITE_VIEW_HPP_ */
