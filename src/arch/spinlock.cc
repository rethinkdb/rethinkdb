#include "arch/spinlock.hpp"

spinlock_t::spinlock_t() {
#if !defined(SPINLOCK_WINDOWS_CRITICAL_SECTION)
#if defined(SPINLOCK_PTHREAD_SPINLOCK)
    int res = pthread_spin_init(&l, PTHREAD_PROCESS_PRIVATE);
#else
    int res = pthread_mutex_init(&l, NULL);
#endif
	guarantee_xerr(res == 0, res, "could not initialize spinlock");
#else
	InitializeCriticalSection(&l);
#endif
}

spinlock_t::~spinlock_t() {
#if !defined(SPINLOCK_WINDOWS_CRITICAL_SECTION)
#if defined(SPINLOCK_PTHREAD_SPINLOCK)
    int res = pthread_spin_destroy(&l);
#else
    int res = pthread_mutex_destroy(&l);
#endif
    guarantee_xerr(res == 0, res, "could not destroy spinlock");
#else
	DeleteCriticalSection(&l);
#endif
}

void spinlock_t::lock() {
#if !defined(SPINLOCK_WINDOWS_CRITICAL_SECTION)
#if defined(SPINLOCK_PTHREAD_SPINLOCK)
	int res = pthread_spin_lock(&l);
#else
    int res = pthread_mutex_lock(&l);
#endif
    guarantee_xerr(res == 0, res, "could not lock spin lock");
#else
	EnterCriticalSection(&l);
#endif
}

void spinlock_t::unlock() {
#if !defined(SPINLOCK_WINDOWS_CRITICAL_SECTION)
#if defined(SPINLOCK_PTHREAD_SPINLOCK)
	int res = pthread_spin_unlock(&l);
#else
    int res = pthread_mutex_unlock(&l);
#endif
    guarantee_xerr(res == 0, res, "could not unlock spin lock");
#else
	LeaveCriticalSection(&l);
#endif
}
