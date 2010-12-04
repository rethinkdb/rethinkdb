#include "coroutine/coroutines.hpp"
#include <stdio.h>

__thread coro_t *coro_t::current_coro = NULL;
__thread coro_t *coro_t::dead_coro = NULL;
__thread coro_t *coro_t::scheduler = NULL;
__thread std::list<coro_t*> *coro_t::notified_coros = NULL;

coro_t *coro_t::self() {
    return current_coro;
}

void coro_t::wait() {
    current_coro->switch_to(scheduler);
}

void coro_t::switch_to(coro_t *next) {
    current_coro = next;
    //printf("Now I'll be %zx\n", (size_t)next);
    Coro_switchTo_(this->underlying, next->underlying);
}

void coro_t::notify() {
#ifndef NDEBUG
    assert(!notified);
    notified = 1;
#endif
    notified_coros->push_back(this);
    //printf("The notified coros are:\n");
    //for (std::list<coro_t*>::iterator iter = notified_coros->begin(); iter != notified_coros->end(); iter++)
        //printf("   %zx\n", (size_t)(*iter));
}

void coro_t::resume() {
    //printf("Resuming %zx\n", (size_t)this);
#ifndef NDEBUG
    assert(notified);
    notified = 0;
#endif
    scheduler->switch_to(this);
}

void coro_t::suicide() {
    //printf("I'm killing myself: %zx\n", (size_t)current_coro);
    dead_coro = current_coro;
    coro_t::wait();
}

struct coro_initialization_data {
    void *arg;
    void (*fn)(void *);
    coro_t *coroutine;
    coro_t *parent;
};

void coro_t::run_coroutine(void *data) {
    coro_initialization_data information = *(coro_initialization_data *)data;
    //printf("Launching the coroutine %zx\n", (size_t)information.coroutine);
    information.coroutine->notify();
    information.coroutine->switch_to(information.parent);
    //printf("Doing the coroutine's action\n");
    (information.fn)(information.arg);
    coro_t::suicide();
}

coro_t::coro_t(void (*fn)(void *arg), void *arg)
    : underlying(Coro_new())
#ifndef NDEBUG
    , notified(0)
#endif
{
    //printf("Making a coroutine\n");
    coro_initialization_data data;
    data.fn = fn;
    data.arg = arg;
    coro_t *previous_coro = data.parent = current_coro;
    data.coroutine = current_coro = this;
    Coro_startCoro_(previous_coro->underlying, underlying, &data, run_coroutine);
}

coro_t::~coro_t() {
    Coro_free(underlying);
}

void execute(void (*arg)()) {
    arg();
}

void coro_t::run_scheduler(void *arg) {
    //This should be the main scheduler loop,
    //and it should launch a coroutine for arg
    //printf("Hi, I'm the scheduler, at %zx\n", (size_t)coro_t::self());
    //printf("Setting up begin\n");
    new coro_t((void (*)(void *))execute, arg);
    while (notified_coros->size() > 0) {
        //printf("Waking up a coroutine\n");
        coro_t *next = notified_coros->front();
        notified_coros->pop_front();
        next->resume();
        //printf("That coroutine decided to wait\n");
        if (dead_coro != NULL) {
            //printf("Deleting a dead coroutine\n");
            delete dead_coro;
            dead_coro = NULL;
            //printf("Deleted!\n");
        }
    }
    //printf("That's all\n");
    exit(0);
}

void coro_t::launch_scheduler(void (*fn)()) {
   Coro *mainCoro = Coro_new();
   Coro_initializeMainCoro(mainCoro);

   notified_coros = new std::list<coro_t*>();

   scheduler = current_coro = new coro_t(Coro_new());
   Coro_startCoro_(mainCoro, scheduler->underlying, (void *)fn, coro_t::run_scheduler);
}

void *task_t::join() {
    assert(waiters.size() == 0 && callbacks.size() == 0);
    //printf("Joining\n");
    if (!done) {
        //printf("Not yet done\n");
        waiters.push_back(coro_t::self());
        //printf("Pushed\n");
        coro_t::wait();
        //printf("Waited\n");
    }
    void *value = result;
    delete this;
    //printf("Returning\n");
    return value;
}

void task_t::callback(task_callback_t *cb) {
    assert(waiters.size() == 0 && callbacks.size() == 0);
    if (done) {
        cb->on_task_return(result);
        delete this;
    } else
        callbacks.push_back(cb);
}

struct run_task_data {
    void *(*fn)(void *);
    void *arg;
    task_t *task;
};

void task_t::run_task(void *arg) {
    //printf("A task is running\n");
    run_task_data data = *(run_task_data *)arg;
    delete (run_task_data *)arg;
    //printf("Executing it\n");
    data.task->result = (data.fn)(data.arg);
    data.task->done = 1;
    //printf("It's done now; notifying joiner\n");
    for (std::vector<coro_t*>::iterator iter = data.task->waiters.begin();
         iter != data.task->waiters.end();
         iter++) {
        (*iter)->notify();
    }
    for (std::vector<task_callback_t*>::iterator iter = data.task->callbacks.begin();
         iter != data.task->callbacks.end();
         iter++) {
        (*iter)->on_task_return(data.task->result);
    }
}

task_t::task_t(void *(*fn)(void *), void *arg)
  : coroutine(NULL), done(0), result(NULL), waiters(), callbacks() {
    run_task_data *data = new run_task_data();
    data->fn = fn;
    data->arg = arg;
    data->task = this;
    //printf("Making a task\n");
    coroutine = new coro_t(run_task, data);
    //printf("Notifying a task\n");
    notify();
}

void task_t::notify() {
    coroutine->notify();
}

void begin();

//int main() {
//   coro_t::launch_scheduler(begin);
//}

//Test code

/*
void other(coro_t *begin_coro) {
    for (int j = 10; j < 15; j++) {
        printf("The second coroutine is at %d\n", j);
        begin_coro->notify();
        coro_t::wait();
    }
    printf("I'm done first!\n");
}

void begin() {
    //Do some stuff with this coroutine library
    //printf("Setting up other and passing it self, which is %zx\n", (size_t)coro_t::self());
    coro_t *other_coro = new coro_t((void (*)(void *))other, coro_t::self());
    for (int i = 0; i < 5; i++) {
        printf("The first coroutine is at %d\n", i);
        other_coro->notify();
        coro_t::wait();
    }
    printf("I'm done!\n");
}
*/
/*
void begin() {
    printf("Hey\n");
}
*/

void *addone(void *arg) {
    printf("Adding one\n");
    return (void *)(((size_t)arg) + 1);
}

void begin() {
    size_t one = 1;
    printf("Dispatching a task\n");
    task_t *task = new task_t(addone, (void *)one);
    printf("Joining it\n");
    void *value = task->join();
    printf("We got %zd\n", (size_t)value);
}
