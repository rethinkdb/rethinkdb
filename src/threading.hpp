/// @file threading.hpp
/// @brief Threading infrastructure and thread-specific utilities
///
/// Provides thread identity management and thread-affinity mixins to ensure
/// objects are only accessed from the correct thread. Essential for safe
/// concurrent access in RethinkDB's multi-threaded architecture.
///
/// @defgroup ThreadingUtilities Threading and Thread Affinity
/// Thread management, thread-local storage, and thread safety
/// @{

#ifndef THREADING_HPP_
#define THREADING_HPP_

#include <stdint.h>
#include <unistd.h>

#include <functional>
#include <vector>

#include "errors.hpp"

/// @brief Thread identifier in RethinkDB thread pool
///
/// Represents a unique thread number within the thread pool.
/// Used to identify and manage individual worker threads.
///
/// @example
/// @code
/// threadnum_t main_thread(0);
/// threadnum_t worker_thread(1);
/// if (main_thread == worker_thread) {
///     // Same thread
/// }
/// @endcode
class threadnum_t {
public:
    /// @brief Constructs a thread ID from an integer
    /// @param _threadnum The thread number (0-based index in thread pool)
    explicit threadnum_t(int32_t _threadnum) : threadnum(_threadnum) { }

    /// @brief Equality comparison for thread IDs
    /// @param other The thread ID to compare with
    /// @return true if both refer to the same thread
    bool operator==(threadnum_t other) const { return threadnum == other.threadnum; }

    /// @brief Inequality comparison for thread IDs
    /// @param other The thread ID to compare with
    /// @return true if they refer to different threads
    bool operator!=(threadnum_t other) const { return !(*this == other); }

    /// @internal The actual thread number
    int32_t threadnum;
};

/// @brief Sentinel value for invalid or uninitialized thread ID
/// Used to mark unset or invalid thread identifiers
static const threadnum_t INVALID_THREAD = threadnum_t(-1);

/// @brief Base mixin for debug-only thread affinity checking
///
/// Objects that inherit from this mixin can only be used on a single thread.
/// In debug builds, assert_thread() verifies correct thread access.
/// In release builds, assertions are no-ops for performance.
///
/// @example
/// @code
/// class MyResource : public home_thread_mixin_debug_only_t {
/// public:
///     MyResource() { }
///     void do_work() {
///         assert_thread();  // Verify we're on the home thread
///         // ... perform thread-safe work ...
///     }
/// };
/// @endcode
class home_thread_mixin_debug_only_t {
public:
/// @brief Verifies that the current execution is on the home thread
/// In debug builds, crashes if called from a different thread.
/// In release builds, this is a no-op for performance.
#ifndef NDEBUG
    void assert_thread() const;
#else
    void assert_thread() const { }
#endif

protected:
    /// @brief Constructs with explicit home thread specification
    /// @param specified_home_thread The thread this object belongs to
    explicit home_thread_mixin_debug_only_t(threadnum_t specified_home_thread);

    /// @brief Default constructor sets home thread to current thread
    home_thread_mixin_debug_only_t();

    /// @internal Virtual destructor for proper cleanup
    ~home_thread_mixin_debug_only_t() { }

#ifndef NDEBUG
    /// @internal The actual home thread (debug builds only)
    threadnum_t real_home_thread;
#endif

    // Objects with home threads should not be copyable to avoid confusion
    // about which thread a copy should belong to
    DISABLE_COPYING(home_thread_mixin_debug_only_t);
};

/// @brief Full-featured mixin for thread affinity with public thread query
///
/// Like home_thread_mixin_debug_only_t but provides a public home_thread()
/// method to query the thread affinity of an object.
/// Useful for objects that need to be explicitly routed to their home thread.
///
/// @example
/// @code
/// class NetworkConnection : public home_thread_mixin_t {
/// public:
///     void send_data(const std::string& data) {
///         assert_thread();  // Verify thread safety
///         // ... send data ...
///     }
/// };
/// 
/// auto conn = std::make_unique<NetworkConnection>();
/// threadnum_t thread_for_io = conn->home_thread();
/// @endcode
class home_thread_mixin_t {
public:
    /// @brief Returns the home thread of this object
    /// Objects should only be accessed from the thread returned by this method
    /// @return The thread ID of the home thread
    threadnum_t home_thread() const { return real_home_thread; }

    /// @brief Verifies that the current execution is on the home thread
    /// In debug builds, crashes if called from a different thread.
    /// In release builds, this is a no-op for performance.
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

/* Tries to distribute allocations evenly across the db threads.
Uses secondary_lt as a tie breaker. */
class thread_allocator_t : public home_thread_mixin_t {
public:
    explicit thread_allocator_t(
        const std::function<bool(threadnum_t, threadnum_t)> &secondary_lt);
    ~thread_allocator_t();
private:
    std::function<bool(threadnum_t, threadnum_t)> secondary_lt;
    std::vector<size_t> num_allocated;
    friend class thread_allocation_t;
    DISABLE_COPYING(thread_allocator_t);
};

class thread_allocation_t {
public:
    explicit thread_allocation_t(thread_allocator_t *p);
    ~thread_allocation_t();
    threadnum_t get_thread() const;
private:
    threadnum_t thread;
    thread_allocator_t *parent;
    DISABLE_COPYING(thread_allocation_t);
};


#endif  // THREADING_HPP_
