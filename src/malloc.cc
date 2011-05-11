#ifdef MALLOC_PROF
#include <google/tcmalloc.h>
#include <stdio.h>
#include <perfmon.hpp>

perfmon_duration_sampler_t pm_operator_new("operator_new", secs_to_ticks(1.0)),
                           pm_operator_delete("operator_delete", secs_to_ticks(1.0));

void* operator new(size_t size) {
    block_pm_duration set_timer(&pm_operator_new);
    return tc_new(size);       
}
void operator delete(void* p) __THROW {
    block_pm_duration set_timer(&pm_operator_delete);
    tc_delete(p);              
}
void* operator new[](size_t size) {
    block_pm_duration set_timer(&pm_operator_new);
    return tc_newarray(size);  
}
void operator delete[](void* p) __THROW {
    block_pm_duration set_timer(&pm_operator_delete);
    tc_deletearray(p);         
}
#endif
/* void* operator new(size_t size, const std::nothrow_t& nt) __THROW {
  return tc_new_nothrow(size, nt);
}
void* operator new[](size_t size, const std::nothrow_t& nt) __THROW {
  return tc_newarray_nothrow(size, nt);
}
void operator delete(void* ptr, const std::nothrow_t& nt) __THROW {
  return tc_delete_nothrow(ptr, nt);
}
void operator delete[](void* ptr, const std::nothrow_t& nt) __THROW {
  return tc_deletearray_nothrow(ptr, nt);
} */
