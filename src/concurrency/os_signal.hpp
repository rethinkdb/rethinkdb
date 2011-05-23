#ifndef __OS_SIGNAL__
#define __OS_SIGNAL__

#include "concurrency/cond_var.hpp"
#include "arch/core.hpp"

void wait_for_sigint();
bool sigint_has_happened();

#endif
