// Copyright Evgeny Panasyuk 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// e-mail: E?????[dot]P???????[at]gmail.???

// Full emulation of await feature from C# language in C++ based on Stackful Coroutines from
// Boost.Coroutine library.
// This proof-of-concept shows that exact syntax of await feature can be emulated with help of
// Stackful Coroutines, demonstrating that it is superior mechanism.
// Main aim of this proof-of-concept is to draw attention to Stackful Coroutines.

#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#define BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_RESULT_OF_USE_DECLTYPE

#include <boost/coroutine/all.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>

#include <functional>
#include <iostream>
#include <cstddef>
#include <utility>
#include <cstdlib>
#include <memory>
#include <vector>
#include <stack>
#include <queue>
#include <ctime>

using namespace std;
using namespace boost;

// ___________________________________________________________ //

template<typename T>
class concurrent_queue
{
    queue<T> q;
    mutex m;
    condition_variable c;
public:
    template<typename U>
    void push(U &&u)
    {
        lock_guard<mutex> l(m);
        q.push( forward<U>(u) );
        c.notify_one();
    }
    void pop(T &result)
    {
        unique_lock<mutex> u(m);
        c.wait(u, [&]{return !q.empty();} );
        result = move_if_noexcept(q.front());
        q.pop();
    }
};

typedef std::function<void()> Task;
concurrent_queue<Task> main_tasks;
auto finished = false;

void reschedule()
{
    this_thread::sleep_for(chrono::milliseconds( rand() % 2000 ));
}

// ___________________________________________________________ //

#ifdef BOOST_COROUTINES_UNIDIRECT
typedef coroutines::coroutine<void>::pull_type      coro_pull;
typedef coroutines::coroutine<void>::push_type      coro_push;
#else
typedef coroutines::coroutine<void()>               coro_pull;
typedef coroutines::coroutine<void()>::caller_type  coro_push;
#endif
struct CurrentCoro
{
    std::shared_ptr<coro_pull> coro;
    coro_push *caller;
};
/*should be thread_local*/ stack<CurrentCoro> coro_stack;

template<typename F>
auto asynchronous(F f) -> future<decltype(f())>
{
    typedef promise<decltype(f())> CoroPromise;

    CoroPromise coro_promise;
    auto coro_future = coro_promise.get_future();

    // It is possible to avoid shared_ptr and use move-semantic,
    //     but it would require to refuse use of std::function (it requires CopyConstructable),
    //     and would lead to further complication and is unjustified
    //     for purposes of this proof-of-concept
    CurrentCoro current_coro =
    {
        make_shared<coro_pull>(std::bind( [f](CoroPromise &coro_promise, coro_push &caller)
        {
            caller();
            coro_stack.top().caller = &caller;
            coro_promise.set_value( f() );
        }, std::move(coro_promise), placeholders::_1 ))
    };

    coro_stack.push( std::move(current_coro) );
    (*coro_stack.top().coro)();
    coro_stack.pop();

#ifdef _MSC_VER
    return std::move( coro_future );
#else
    return coro_future;
#endif
}

struct Awaiter
{
    template<typename Future>
    auto operator*(Future &&ft) -> decltype(ft.get())
    {
        typedef decltype(ft.get()) Result;

        auto &&current_coro = coro_stack.top();
        auto result = ft.then([current_coro](Future &ft) -> Result
        {
            main_tasks.push([current_coro]
            {
                coro_stack.push(std::move(current_coro));
                (*coro_stack.top().coro)();
                coro_stack.pop();
            });
            return ft.get();
        });
        (*coro_stack.top().caller)();
        return result.get();
    }
};
#define await Awaiter()*

// ___________________________________________________________ //

void async_user_handler();

int main()
{
    srand(time(0));

    // Custom scheduling is not required - can be integrated
    // to other systems transparently
    main_tasks.push([]
    {
        asynchronous([]
        {
            return async_user_handler(),
                   finished = true;
        });
    });

    Task task;
    while(!finished)
    {
        main_tasks.pop(task);
        task();
    }
}

// __________________________________________________________________ //

int bar(int i)
{
    // await is not limited by "one level" as in C#
    auto result = await async([i]{ return reschedule(), i*100; });
    return result + i*10;
}

int foo(int i)
{
    cout << i << ":\tbegin" << endl;
    cout << await async([i]{ return reschedule(), i*10; }) << ":\tbody" << endl;
    cout << bar(i) << ":\tend" << endl;
    return i*1000;
}

void async_user_handler()
{
    vector<future<int>> fs;

    for(auto i=0; i!=5; ++i)
        fs.push_back( asynchronous([i]{ return foo(i+1); }) );

    for(auto &&f : fs)
        cout << await f << ":\tafter end" << endl;
}
