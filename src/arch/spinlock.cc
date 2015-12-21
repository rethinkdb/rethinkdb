#include "arch/spinlock.hpp"

spinlock_t::spinlock_t() {
#if defined(SPINLOCK_PTHREAD_SPINLOCK)
    int res = pthread_spin_init(&l, PTHREAD_PROCESS_PRIVATE);
    guarantee_xerr(res == 0, res, "could not initialize spinlock");
#elif defined(SPINLOCK_PTHREAD_MUTEX)
    int res = pthread_mutex_init(&l, NULL);
    guarantee_xerr(res == 0, res, "could not initialize spinlock");
#endif
}

spinlock_t::~spinlock_t() {
#if defined(SPINLOCK_PTHREAD_SPINLOCK)
    int res = pthread_spin_destroy(&l);
#elif defined(SPINLOCK_PTHREAD_MUTEX)
    int res = pthread_mutex_destroy(&l);
#endif
    guarantee_xerr(res == 0, res, "could not destroy spinlock");
}

void spinlock_t::lock() {
#if defined(SPINLOCK_PTHREAD_SPINLOCK)
	int res = pthread_spin_lock(&l);
#elif defined(SPINLOCK_PTHREAD_MUTEX)
    int res = pthread_mutex_lock(&l);
#endif
    guarantee_xerr(res == 0, res, "could not lock spin lock");
}

void spinlock_t::unlock() {
#if defined(SPINLOCK_PTHREAD_SPINLOCK)
    int res = pthread_spin_unlock(&l);
#elif defined(SPINLOCK_PTHREAD_MUTEX)
    int res = pthread_mutex_unlock(&l);
#endif
    guarantee_xerr(res == 0, res, "could not unlock spin lock");
}
