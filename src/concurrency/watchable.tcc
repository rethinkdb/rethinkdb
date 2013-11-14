// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "concurrency/watchable.hpp"

#include <functional>

#include "errors.hpp"
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include "concurrency/cond_var.hpp"
#include "concurrency/wait_any.hpp"

template <class outer_type, class callable_type>
class subview_watchable_t : public watchable_t<typename boost::result_of<callable_type(outer_type)>::type> {
public:
    typedef typename boost::result_of<callable_type(outer_type)>::type result_type;

    subview_watchable_t(const callable_type &l, watchable_t<outer_type> *p) :
        cache(new lensed_value_cache_t(l, p)) {
    }

    subview_watchable_t *clone() const {
        return new subview_watchable_t(cache);
    }

    result_type get() {
        return *cache->get();
    }

    void apply_atomic_op(const std::function<bool(result_type*)> &op) {
        ASSERT_NO_CORO_WAITING;
        guarantee(op(cache->get()) == false);
    }

    virtual void apply_read(const std::function<void(const result_type*)> &read) {
        ASSERT_NO_CORO_WAITING;
        read(cache->get());
    }

    publisher_t<boost::function<void()> > *get_publisher() {
        return cache->parent->get_publisher();
    }

    rwi_lock_assertion_t *get_rwi_lock_assertion() {
        return cache->parent->get_rwi_lock_assertion();
    }

private:
    // TODO (daniel): Document how incremental lenses work.
    class lensed_value_cache_t {
    public:
        lensed_value_cache_t(const callable_type &l, watchable_t<outer_type> *p) :
            parent(p->clone()),
            lens(l),
            parent_subscription(boost::bind(
                &subview_watchable_t<outer_type, callable_type>::lensed_value_cache_t::on_parent_changed,
                this)) {

            typename watchable_t<outer_type>::freeze_t freeze(parent);
            parent_changed = true; // Initializing here, so we definitely set the flag
                                   // while having the parent freezed.
            parent_subscription.reset(parent, &freeze);
        }

        ~lensed_value_cache_t() {
            {
                on_thread_t t(parent->home_thread());
                parent_subscription.reset();
            }
        }

        result_type *get() {
            if (parent_changed) {
                // TODO: Some kind of locking maybe?
                compute_value();
            }
            return &cached_value;
        }

        clone_ptr_t<watchable_t<outer_type> > parent;

    private:
        void compute_value() {
            // The closure is to avoid copying the whole value from the parent.

            /* C++11: auto op = [&] (const outer_type *val) -> void { ... }
            Because we cannot use C++11 lambdas yet due to missing support in
            GCC 4.4, this is the messy work-around: */
            struct op_closure_t {
                void operator()(const outer_type *val) {
                    parent_changed = false;
                    cached_value = lens(*val, static_cast<const result_type *>(&cached_value));
                }
                op_closure_t(callable_type &c1, bool &c2, result_type &c3) :
                    lens(c1),
                    parent_changed(c2),
                    cached_value(c3) {
                }
                callable_type &lens;
                bool &parent_changed;
                result_type &cached_value;
            };
            op_closure_t op(lens, parent_changed, cached_value);

            parent->apply_read(std::bind(&op_closure_t::operator(), &op, std::placeholders::_1));
        }

        void on_parent_changed() {
            parent_changed = true;
        }

        callable_type lens;
        bool parent_changed;
        result_type cached_value;
        typename watchable_t<outer_type>::subscription_t parent_subscription;
    };

    subview_watchable_t(const boost::shared_ptr<lensed_value_cache_t> &_cache) :
        cache(_cache) {
    }

    boost::shared_ptr<lensed_value_cache_t> cache;
};

template<class value_type>
template<class callable_type>
clone_ptr_t<watchable_t<typename boost::result_of<callable_type(value_type)>::type> > watchable_t<value_type>::subview(const callable_type &lens) {
    assert_thread();
    return clone_ptr_t<watchable_t<typename boost::result_of<callable_type(value_type)>::type> >(
        new subview_watchable_t<value_type, callable_type>(lens, this));
}

template<class value_type>
template<class callable_type>
void watchable_t<value_type>::run_until_satisfied(const callable_type &fun, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    assert_thread();
    clone_ptr_t<watchable_t<value_type> > clone_this(this->clone());
    while (true) {
        cond_t changed;
        typename watchable_t<value_type>::subscription_t subs(boost::bind(&cond_t::pulse_if_not_already_pulsed, &changed));
        {
            typename watchable_t<value_type>::freeze_t freeze(clone_this);
            ASSERT_FINITE_CORO_WAITING;
            if (fun(clone_this->get())) {
                return;
            }
            subs.reset(clone_this, &freeze);
        }
        wait_interruptible(&changed, interruptor);
    }
}

template<class a_type, class b_type, class callable_type>
void run_until_satisfied_2(
        const clone_ptr_t<watchable_t<a_type> > &a,
        const clone_ptr_t<watchable_t<b_type> > &b,
        const callable_type &fun,
        signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    a->assert_thread();
    b->assert_thread();
    while (true) {
        cond_t changed;
        typename watchable_t<a_type>::subscription_t a_subs(boost::bind(&cond_t::pulse_if_not_already_pulsed, &changed));
        typename watchable_t<b_type>::subscription_t b_subs(boost::bind(&cond_t::pulse_if_not_already_pulsed, &changed));
        {
            typename watchable_t<a_type>::freeze_t a_freeze(a);
            typename watchable_t<b_type>::freeze_t b_freeze(b);
            ASSERT_FINITE_CORO_WAITING;
            if (fun(a->get(), b->get())) {
                return;
            }
            a_subs.reset(a, &a_freeze);
            b_subs.reset(b, &b_freeze);
        }
        wait_interruptible(&changed, interruptor);
    }
}
