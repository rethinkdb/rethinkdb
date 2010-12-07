#include "coroutine/coroutines.hpp"
#include <stdio.h>

__thread coro_t *coro_t::current_coro = NULL;
__thread coro_t *coro_t::scheduler = NULL;

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
    notified = true;
#endif
    call_later_on_this_cpu(this);
    //printf("The notified coros are:\n");
    //for (std::list<coro_t*>::iterator iter = notified_coros->begin(); iter != notified_coros->end(); iter++)
        //printf("   %zx\n", (size_t)(*iter));
}

void coro_t::on_cpu_switch() {
    //printf("Resuming %zx\n", (size_t)this);
#ifndef NDEBUG
    assert(notified);
    notified = false;
#endif
    scheduler->switch_to(this);
    if (dead) delete this;
}

void coro_t::suicide() {
    //printf("I'm killing myself: %zx\n", (size_t)current_coro);
    self()->dead = true;
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
#ifndef NDEBUG
    information.coroutine->notified = false;
#endif
    (information.fn)(information.arg);
    coro_t::suicide();
}

coro_t::coro_t(void (*fn)(void *arg), void *arg)
    : underlying(Coro_new()), dead(false)
#ifndef NDEBUG
    , notified(false)
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

void coro_t::run() {
    Coro *mainCoro = Coro_new();
    Coro_initializeMainCoro(mainCoro);
    scheduler = current_coro = new coro_t(mainCoro);
}

void coro_t::destroy() {
    delete scheduler;
    scheduler = current_coro = NULL;
}

void *task_t::join() {
    assert(waiters.size() == 0);
    //printf("Joining\n");
    if (!done) {
        //printf("Not yet done\n");
        waiters.push_back(coro_t::self());
        //printf("Pushed\n");
        coro_t::wait();
        assert(done);
        //printf("Waited\n");
    }
    void *value = result;
    delete this;
    //printf("Returning\n");
    return value;
}

struct task_callback_data {
    task_callback_t *cb;
    task_t *task;
};

void task_t::run_callback(void *arg) {
    task_callback_data *data = (task_callback_data *)arg;
    data->cb->on_task_return(data->task->join());
    delete data;
}

void task_t::callback(task_callback_t *cb) {
    task_callback_data *data = new task_callback_data();
    data->cb = cb;
    data->task = this;
    new coro_t(&run_callback, data);
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
}

task_t::task_t(void *(*fn)(void *), void *arg)
  : coroutine(NULL), done(0), result(NULL), waiters() {
    run_task_data *data = new run_task_data();
    data->fn = fn;
    data->arg = arg;
    data->task = this;
    //printf("Making a task\n");
    coroutine = new coro_t(run_task, data);
    //printf("Notifying a task\n");
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
