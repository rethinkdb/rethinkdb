// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "concurrency/watchable.hpp"

#include <functional>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "arch/timing.hpp"
#include "concurrency/cond_var.hpp"
#include "concurrency/wait_any.hpp"

template <class result_type, class outer_type, class callable_type>
class subview_watchable_t : public watchable_t<result_type> {
public:
    subview_watchable_t(const callable_type &l, watchable_t<outer_type> *p) :
        cache(new lensed_value_cache_t(l, p)) { }

    subview_watchable_t *clone() const {
        return new subview_watchable_t(cache);
    }

    result_type get() {
        return *cache->get();
    }

    virtual void apply_read(const std::function<void(const result_type*)> &read) {
        ASSERT_NO_CORO_WAITING;
        read(cache->get());
    }

    publisher_t<std::function<void()> > *get_publisher() {
        return cache->get_publisher();
    }

    rwi_lock_assertion_t *get_rwi_lock_assertion() {
        return cache->parent->get_rwi_lock_assertion();
    }

private:
    // lensed_value_cache_t is a cache for the result of a lens applied to
    // a given parent watchable. It is implemented for incremental lenses only.
    // (`non_incremental_lens_wrapper_t` can be used to apply it to non-incremental
    // lenses as well).
    // The cached value is recomputed only after the parent has published a new
    // value.
    class lensed_value_cache_t : public home_thread_mixin_t {
    public:
        lensed_value_cache_t(const callable_type &l, watchable_t<outer_type> *p) :
            parent(p->clone()),
            lens(l),
            parent_subscription(std::bind(
                &subview_watchable_t<result_type, outer_type, callable_type>::lensed_value_cache_t::on_parent_changed,
                this)) {
            typename watchable_t<outer_type>::freeze_t freeze(parent);
            ASSERT_FINITE_CORO_WAITING;
            compute_value();
            parent_subscription.reset(parent, &freeze);
        }

        ~lensed_value_cache_t() {
            assert_thread();
        }

        const result_type *get() {
            assert_thread();
            return &cached_value;
        }

        publisher_t<std::function<void()> > *get_publisher() {
            assert_thread();
            return publisher_controller.get_publisher();
        }

        clone_ptr_t<watchable_t<outer_type> > parent;

    private:
        bool compute_value() {
            // The closure is to avoid copying the whole value from the parent.

            bool value_changed = false;
            struct op_closure_t {
                static void apply(callable_type *_lens,
                                  bool *_value_changed,
                                  result_type *_cached_value,
                                  const outer_type *val) {
                    *_value_changed = (*_lens)(*val, _cached_value);
                }
            };

            parent->apply_read(std::bind(&op_closure_t::apply,
                                         &lens,
                                         &value_changed,
                                         &cached_value,
                                         std::placeholders::_1));

            return value_changed;
        }

        void on_parent_changed() {
            assert_thread();
            const bool value_changed = compute_value();
            if (value_changed) {
                publisher_controller.publish(&call_function);
            }
        }

        callable_type lens;
        result_type cached_value;
        publisher_controller_t<std::function<void()> > publisher_controller;
        typename watchable_t<outer_type>::subscription_t parent_subscription;
    };

    explicit subview_watchable_t(const boost::shared_ptr<lensed_value_cache_t> &_cache) :
        cache(_cache) { }

    // If you clone a subview_watchable_t, all clones share the same cache.
    // As a consequence, the lens is only applied once for the whole family
    // of clones, not once per instance of subview_watchable_t. This should save
    // some CPU and memory as well.
    boost::shared_ptr<lensed_value_cache_t> cache;
};

/* Given a non-incremental lens with the type signature
 * `result_type(input_type)`, `non_incremental_lens_wrapper_t` converts
 * it into an incremental lens with type signature
 * `bool(const input_type &, result_type *)` and otherwise equivalent semantics
 * (as much as possible). */
template<class outer_type, class callable_type>
class non_incremental_lens_wrapper_t {
public:
    typedef typename std::result_of<callable_type(outer_type)>::type result_type;

    explicit non_incremental_lens_wrapper_t(const callable_type &_inner) :
        inner(_inner) {
    }

    bool operator()(const outer_type &input, result_type *current_out) {
        guarantee(current_out != NULL);
        result_type old_value = *current_out;
        *current_out = inner(input);
        return old_value != *current_out;
    }
private:
    callable_type inner;
};

template<class value_type>
template<class callable_type>
clone_ptr_t<watchable_t<typename callable_type::result_type> > watchable_t<value_type>::incremental_subview(const callable_type &lens) {
    assert_thread();
    return clone_ptr_t<watchable_t<typename callable_type::result_type> >(
        new subview_watchable_t<typename callable_type::result_type, value_type, callable_type>(lens, this));
}

template<class value_type>
template<class result_type, class callable_type>
clone_ptr_t<watchable_t<result_type> > watchable_t<value_type>::incremental_subview(const callable_type &lens) {
    assert_thread();
    return clone_ptr_t<watchable_t<result_type> >(
        new subview_watchable_t<result_type, value_type, callable_type>(lens, this));
}

template<class value_type>
template<class callable_type>
clone_ptr_t<watchable_t<typename std::result_of<callable_type(value_type)>::type> > watchable_t<value_type>::subview(const callable_type &lens) {
    assert_thread();
    typedef non_incremental_lens_wrapper_t<value_type, callable_type> wrapped_callable_type;
    wrapped_callable_type wrapped_lens(lens);
    return clone_ptr_t<watchable_t<typename wrapped_callable_type::result_type> >(
        new subview_watchable_t<typename wrapped_callable_type::result_type, value_type, wrapped_callable_type>(wrapped_lens, this));
}

// These are helper functions for `run_until_satisfied()` and `run_until_satisfied_2()`
// that can be passed into a watchable's `apply_read()`. They simply apply the
// provided function `_fun` to the current value of the watchable(s) and pass out
// its boolean return value.
template<class value_type, class callable_type>
void run_until_satisfied_apply(const callable_type &_fun, const value_type *val,
        bool *is_done_out) {
    *is_done_out = _fun(*val);
}

template<class value_type>
template<class callable_type>
void watchable_t<value_type>::run_until_satisfied(const callable_type &fun,
        signal_t *interruptor, int64_t nap_before_retry_ms) THROWS_ONLY(interrupted_exc_t) {
    assert_thread();

    bool is_done = false;
    auto op = std::bind(&run_until_satisfied_apply<value_type, callable_type>,
                        fun,
                        std::placeholders::_1,
                        &is_done);

    clone_ptr_t<watchable_t<value_type> > clone_this(this->clone());
    while (true) {
        cond_t changed;
        typename watchable_t<value_type>::subscription_t subs(std::bind(&cond_t::pulse_if_not_already_pulsed, &changed));
        {
            typename watchable_t<value_type>::freeze_t freeze(clone_this);
            ASSERT_FINITE_CORO_WAITING;
            clone_this->apply_read(op);
            if (is_done) {
                return;
            }
            subs.reset(clone_this, &freeze);
        }
        // Nap a little so changes to the watchables can accumulate.
        // This is purely a performance optimization to save CPU cycles,
        // in case that applying `fun` is expensive.
        if (nap_before_retry_ms > 0) {
            nap(nap_before_retry_ms, interruptor);
        }
        wait_interruptible(&changed, interruptor);
    }
}

