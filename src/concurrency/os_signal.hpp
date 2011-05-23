#ifndef __OS_SIGNAL__
#define __OS_SIGNAL__

#include "concurrency/cond_var.hpp"
#include "arch/core.hpp"

void wait_for_sigint();

class sigint_indicator_t {
private:
    bool _sigint_has_happened;
public:
    sigint_indicator_t() 
        : _sigint_has_happened(false) //this has to be constructed before sigints
    { 
        coro_t::spawn(boost::bind(&sigint_indicator_t::watch_for_sigint, this));
    }
    bool sigint_has_happened() { return _sigint_has_happened; }
private:
    void watch_for_sigint() {
        wait_for_sigint();
        _sigint_has_happened = true;
    }
};

#endif
