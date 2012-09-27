#include "concurrency/watchable.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "concurrency/cond_var.hpp"
#include "concurrency/wait_any.hpp"

template <class outer_type, class callable_type>
class subview_watchable_t : public watchable_t<typename boost::result_of<callable_type(outer_type)>::type> {
public:
    subview_watchable_t(const callable_type &l, watchable_t<outer_type> *p) :
        lens(l), parent(p->clone()) { }

    subview_watchable_t *clone() const {
        return new subview_watchable_t(lens, parent.get());
    }

    typename boost::result_of<callable_type(outer_type)>::type get() {
        return lens(parent->get());
    }

    publisher_t<boost::function<void()> > *get_publisher() {
        return parent->get_publisher();
    }

    rwi_lock_assertion_t *get_rwi_lock_assertion() {
        return parent->get_rwi_lock_assertion();
    }

private:
    callable_type lens;
    clone_ptr_t<watchable_t<outer_type> > parent;
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
