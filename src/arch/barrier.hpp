#ifndef ARCH_BARRIER_HPP_
#define ARCH_BARRIER_HPP_

// We call this a pthread_barrier_t so as to differentiate from other barrier types.
class thread_barrier_t {
public:
    thread_barrier_t(int num_workers);
    ~thread_barrier_t();

    void wait();

private:
    thread_barrier_t barrier_;

    DISABLE_COPYING(thread_barrier_t);
};

#endif  // ARCH_BARRIER_HPP_
