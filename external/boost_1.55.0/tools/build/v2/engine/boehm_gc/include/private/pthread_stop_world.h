#ifndef GC_PTHREAD_STOP_WORLD_H
#define GC_PTHREAD_STOP_WORLD_H

struct thread_stop_info {
    word last_stop_count;	/* GC_last_stop_count value when thread	*/
    				/* last successfully handled a suspend	*/
    				/* signal.				*/
    ptr_t stack_ptr;  		/* Valid only when stopped.      	*/
};
    
#endif
