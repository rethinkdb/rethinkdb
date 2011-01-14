
#ifndef __ARCH_LINUX_TIMER_PROVIDER_CALLBACK_HPP__
#define __ARCH_LINUX_TIMER_PROVIDER_CALLBACK_HPP__

/* Timer provider callback */
struct timer_provider_callback_t {
    virtual void on_timer(int nexpirations) = 0;
};


#endif // __ARCH_LINUX_TIMER_PROVIDER_CALLBACK_HPP__

