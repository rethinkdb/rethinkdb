#ifndef __CONCURRENCY_WATCHABLE_HPP__
#define __CONCURRENCY_WATCHABLE_HPP__

#include "concurrency/mutex_assertion.hpp"
#include "concurrency/pubsub.hpp"
#include "containers/clone_ptr.hpp"

template <class value_t>
class watchable_t {
public:
    class freeze_t {
    public:
        explicit freeze_t(const clone_ptr_t<watchable_t> &watchable) 
            : rwi_lock_acquisition(watchable->get_rwi_lock_assertion())
        { }

        void assert_is_holding(const clone_ptr_t<watchable_t> &watchable) {
            rwi_lock_acquisition.assert_is_holding(watchable->get_rwi_lock_assertion());
        }

    private:
        rwi_lock_assertion_t::read_acq_t rwi_lock_acquisition;
    };

    class subscription_t {
    public:
        explicit subscription_t(boost::function<void()> f)
            : subscription(f)
        { }

        subscription_t(boost::function<void()> f, const clone_ptr_t<watchable_t> &watchable, freeze_t *freeze) 
            : subscription(f, watchable->get_publisher(freeze))
        { }

        void reset() {
            subscription.reset();
        }

        void reset(const clone_ptr_t<watchable_t> &watchable, freeze_t *freeze) {
            subscription.reset(watchable->get_publisher(freeze));
        }

    private:
        typename publisher_t<boost::function<void()> >::subscription_t subscription;
    };

    virtual ~watchable_t() { }
    virtual watchable_t *clone() = 0;
    virtual value_t get() = 0;
    virtual publisher_t<boost::function<void()> > *get_publisher(freeze_t *) = 0;
    virtual rwi_lock_assertion_t *get_rwi_lock_assertion() = 0;

protected:
    watchable_t() { }

private:
    DISABLE_COPYING(watchable_t);
};

template <class value_t>
class watchable_variable_t {
public:
    explicit watchable_variable_t(const value_t &_value)
        : value(_value), watchable(this)
    { }

    clone_ptr_t<watchable_t<value_t> > get_watchable() {
        return clone_ptr_t<watchable_t<value_t> >(watchable.clone());
    }

    void set_value(const value_t &_value) {
        rwi_lock_assertion_t::write_acq_t acquisition(&rwi_lock_assertion);
        value = _value;
        publisher_controller.publish(&watchable_variable_t<value_t>::call);
    }

private:
    class w_t : public watchable_t<value_t> {
    public:
        explicit w_t(watchable_variable_t<value_t> *p) : parent(p) { }
        w_t *clone() {
            return new w_t(parent);
        }
        value_t get() {
            return parent->value;
        }
        publisher_t<boost::function<void()> > *get_publisher(typename watchable_t<value_t>::freeze_t *f) {
            f->assert_is_holding(parent->get_watchable());
            return parent->publisher_controller.get_publisher();
        }
        rwi_lock_assertion_t *get_rwi_lock_assertion() {
            return &parent->rwi_lock_assertion;
        }
        watchable_variable_t<value_t> *parent;
    };

    static void call(const boost::function<void()> &f) {
        f();
    }

    publisher_controller_t<boost::function<void()> > publisher_controller;
    value_t value;
    rwi_lock_assertion_t rwi_lock_assertion;

    w_t watchable;

    DISABLE_COPYING(watchable_variable_t);
};

#endif /* __CONCURRENCY_WATCHABLE_HPP__ */
