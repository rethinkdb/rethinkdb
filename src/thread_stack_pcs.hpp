#ifndef THREAD_STACK_PCS_HPP_
#define THREAD_STACK_PCS_HPP_

#ifdef __MACH__
int rethinkdb_backtrace(void **buffer, int size);
#endif

#endif  // THREAD_STACK_PCS_HPP_
