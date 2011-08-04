#include "arch/runtime/runtime_utils.hpp"

#include <unistd.h>

int get_cpu_count() {
    return sysconf(_SC_NPROCESSORS_ONLN);
}
