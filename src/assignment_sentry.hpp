/**
 * @file assignment_sentry.hpp
 * @brief RAII guard for temporary variable assignments with automatic restoration.
 *
 * Provides a template class for safely managing temporary variable assignments
 * with exception-safe restoration of previous values on scope exit.
 */

#ifndef ASSIGNMENT_SENTRY_HPP_
#define ASSIGNMENT_SENTRY_HPP_

/**
 * @brief RAII guard for temporarily assigning a value to a variable.
 *
 * This template class implements the Resource Acquisition Is Initialization (RAII)
 * pattern to safely manage temporary variable assignments. When the sentry object
 * goes out of scope, the original value is automatically restored, even in the
 * presence of exceptions.
 *
 * @tparam T The type of the variable being guarded.
 *
 * Exception Safety: Provides strong exception safety guarantee. The assignment
 * and restoration operations must not throw.
 *
 * Example:
 * @code
 * int original = 10;
 * {
 *     assignment_sentry_t<int> sentry(&original, 42);
 *     assert(original == 42);  // Value changed
 * }  // Destructor restores original value
 * assert(original == 10);      // Back to original
 * @endcode
 *
 * Example with exception safety:
 * @code
 * int counter = 0;
 * try {
 *     assignment_sentry_t<int> sentry(&counter, 100);
 *     throw std::runtime_error("Error during processing");
 * } catch (...) {
 *     // counter is automatically restored to 0 even after exception
 * }
 * @endcode
 */
template <class T>
class assignment_sentry_t {
public:
    /**
     * @brief Default constructor creating an inactive sentry.
     *
     * The sentry is inactive and will not restore any value when destroyed.
     */
    assignment_sentry_t() : var(nullptr), old_value() { }

    /**
     * @brief Construct and assign a value to a variable.
     *
     * Stores the variable pointer and its current value, then assigns the new value.
     * The original value will be restored when this sentry is destroyed.
     *
     * @param v Pointer to the variable to manage.
     * @param value The new value to assign.
     */
    assignment_sentry_t(T *v, const T &value) :
            var(v), old_value(*var) {
        *var = value;
    }

    /**
     * @brief Destructor restoring the original value.
     *
     * Restores the original value of the managed variable (if one was set).
     * Safe to call even if reset() was previously called.
     */
    ~assignment_sentry_t() {
        reset();
    }

    /**
     * @brief Reset to manage a different variable with a new value.
     *
     * First restores the previously managed variable (if any), then sets up
     * this sentry to manage a new variable with the given value.
     *
     * @param v Pointer to the new variable to manage.
     * @param value The new value to assign to the variable.
     */
    void reset(T *v, const T &value) {
        reset();
        var = v;
        old_value = *var;
        *var = value;
    }

    /**
     * @brief Restore the original value and deactivate this sentry.
     *
     * Restores the original value to the managed variable and deactivates
     * this sentry. Safe to call multiple times.
     */
    void reset() {
        if (var != nullptr) {
            *var = old_value;
            var = nullptr;
        }
    }
private:
    T *var;
    T old_value;
};

#endif  // ASSIGNMENT_SENTRY_HPP_
