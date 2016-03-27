#include "arch/spinlock.hpp"

spinlock_t::spinlock_t() {
#if PTHREAD_HAS_SPINLOCK
    int res = pthread_spin_init(&l, PTHREAD_PROCESS_PRIVATE);
#else
    int res = pthread_mutex_init(&l, nullptr);
#endif
    guarantee_xerr(res == 0, res, "could not initialize spinlock");
}

spinlock_t::~spinlock_t() {
#if PTHREAD_HAS_SPINLOCK
    int res = pthread_spin_destroy(&l);
#else
    int res = pthread_mutex_destroy(&l);
#endif
    guarantee_xerr(res == 0, res, "could not destroy spinlock");
}

void spinlock_t::lock() {
#if PTHREAD_HAS_SPINLOCK
    int res = pthread_spin_lock(&l);
#else
    int res = pthread_mutex_lock(&l);
#endif
    guarantee_xerr(res == 0, res, "could not lock spin lock");
}

void spinlock_t::unlock() {
#if PTHREAD_HAS_SPINLOCK
    int res = pthread_spin_unlock(&l);
#else
    int res = pthread_mutex_unlock(&l);
#endif
    guarantee_xerr(res == 0, res, "could not unlock spin lock");
}
