#ifndef GC_DARWIN_SEMAPHORE_H
#define GC_DARWIN_SEMAPHORE_H

#if !defined(GC_DARWIN_THREADS)
#error darwin_semaphore.h included with GC_DARWIN_THREADS not defined
#endif

/*
   This is a very simple semaphore implementation for darwin. It
   is implemented in terms of pthreads calls so it isn't async signal
   safe. This isn't a problem because signals aren't used to
   suspend threads on darwin.
*/
   
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int value;
} sem_t;

static int sem_init(sem_t *sem, int pshared, int value) {
    int ret;
    if(pshared)
        GC_abort("sem_init with pshared set");
    sem->value = value;
    
    ret = pthread_mutex_init(&sem->mutex,NULL);
    if(ret < 0) return -1;
    ret = pthread_cond_init(&sem->cond,NULL);
    if(ret < 0) return -1;
    return 0;
}

static int sem_post(sem_t *sem) {
    if(pthread_mutex_lock(&sem->mutex) < 0)
        return -1;
    sem->value++;
    if(pthread_cond_signal(&sem->cond) < 0) {
        pthread_mutex_unlock(&sem->mutex);
        return -1;
    }
    if(pthread_mutex_unlock(&sem->mutex) < 0)
        return -1;
    return 0;
}

static int sem_wait(sem_t *sem) {
    if(pthread_mutex_lock(&sem->mutex) < 0)
        return -1;
    while(sem->value == 0) {
        pthread_cond_wait(&sem->cond,&sem->mutex);
    }
    sem->value--;
    if(pthread_mutex_unlock(&sem->mutex) < 0)
        return -1;    
    return 0;
}

static int sem_destroy(sem_t *sem) {
    int ret;
    ret = pthread_cond_destroy(&sem->cond);
    if(ret < 0) return -1;
    ret = pthread_mutex_destroy(&sem->mutex);
    if(ret < 0) return -1;
    return 0;
}

#endif
