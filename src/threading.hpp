#ifndef THREADING_HPP_
#define THREADING_HPP_

#include <stdint.h>

#include "errors.hpp"

// A thread number as used by the thread pool.
class threadnum_t {
public:
    explicit threadnum_t(int32_t _threadnum) : threadnum(_threadnum) { }

    bool operator==(threadnum_t other) const { return threadnum == other.threadnum; }
    bool operator!=(threadnum_t other) const { return !(*this == other); }

    int32_t threadnum;
};

/* The home thread mixin is a mixin for objects that can only be used
on a single thread. Its thread ID is exposed as the `home_thread()`
method. Some subclasses of `home_thread_mixin_debug_only_t` can move themselves to
another thread, modifying the field real_home_thread. */

#define INVALID_THREAD (threadnum_t(-1))

class home_thread_mixin_debug_only_t {
public:
#ifndef NDEBUG
    void assert_thread() const;
#else
    void assert_thread() const { }
#endif

protected:
    explicit home_thread_mixin_debug_only_t(threadnum_t specified_home_thread);
    home_thread_mixin_debug_only_t();
    ~home_thread_mixin_debug_only_t() { }

#ifndef NDEBUG
    threadnum_t real_home_thread;
#endif

    // Things with home threads should not be copyable, since we don't
    // want to nonchalantly copy their real_home_thread variable.
    DISABLE_COPYING(home_thread_mixin_debug_only_t);
};

class home_thread_mixin_t {
public:
    threadnum_t home_thread() const { return real_home_thread; }
#ifndef NDEBUG
    void assert_thread() const;
#else
    void assert_thread() const { }
#endif

protected:
    explicit home_thread_mixin_t(threadnum_t specified_home_thread);
    home_thread_mixin_t();
    home_thread_mixin_t(home_thread_mixin_t &&movee) noexcept
        : real_home_thread(movee.real_home_thread) { }
    ~home_thread_mixin_t() { }

    void operator=(home_thread_mixin_t &&) = delete;

    threadnum_t real_home_thread;

    // Things with home threads should not be copyable, since we don't
    // want to nonchalantly copy their real_home_thread variable.
    DISABLE_COPYING(home_thread_mixin_t);
};

/* `on_thread_t` switches to the given thread in its constructor, then switches
back in its destructor. For example:

    printf("Suppose we are on thread 1.\n");
    {
        on_thread_t thread_switcher(2);
        printf("Now we are on thread 2.\n");
    }
    printf("And now we are on thread 1 again.\n");

*/

class on_thread_t : public home_thread_mixin_t {
public:
    explicit on_thread_t(threadnum_t thread);
    ~on_thread_t();
};

int get_num_db_threads();

#endif  // THREADING_HPP_
