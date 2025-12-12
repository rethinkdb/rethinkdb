/**
 * @file assignment_sentry.hpp
 * @brief RAII wrapper for temporary variable assignments.
 *
 * Provides a template class that implements the RAII pattern to safely manage
 * temporary assignments to variables. When constructed, it saves the original
 * value and assigns a new one. When destructed, it automatically restores
 * the original value.
 */

#ifndef ASSIGNMENT_SENTRY_HPP_
#define ASSIGNMENT_SENTRY_HPP_

/**
 * @defgroup RAIIPatterns RAII Utilities
 * @brief Classes implementing the RAII (Resource Acquisition Is Initialization) pattern.
 */

/**
 * @ingroup RAIIPatterns
 * @brief RAII wrapper for temporary variable assignments.
 *
 * This template class implements the RAII pattern to safely manage temporary
 * changes to variable values. It saves the original value on construction and
 * restores it on destruction, ensuring exception-safe code.
 *
 * @tparam T The type of variable being managed. Must be copy-assignable.
 *
 * @code
 * {
 *     int x = 10;
 *     {
 *         assignment_sentry_t<int> sentry(&x, 20);
 *         // x is now 20
 *         // ... perform operations with x = 20 ...
 *     }  // x is automatically restored to 10 here
 *     assert(x == 10);
 * }
 * @endcode
 *
 * @code
 * // Also useful for managing enum states
 * enum State { IDLE, PROCESSING, DONE };
 * State current_state = IDLE;
 *
 * void process_with_state_protection() {
 *     assignment_sentry_t<State> sentry(&current_state, PROCESSING);
 *     // perform work...
 * }  // current_state automatically restored to IDLE even if exception occurs
 * @endcode
 *
 * The sentry can be reset multiple times during its lifetime:
 *
 * @code
 * assignment_sentry_t<int> sentry;
 * sentry.reset(&x, 5);
 * // x is now 5, and sentry will restore its previous value on destruction
 * sentry.reset();  // manually stop managing x early
 * @endcode
 */
template <class T>
class assignment_sentry_t {
public:
    /**
     * @brief Constructs an empty sentry that doesn't manage any variable.
     */
    assignment_sentry_t() : var(nullptr), old_value() { }

    /**
     * @brief Constructs a sentry that temporarily assigns a new value to a variable.
     *
     * Saves the current value of *v and assigns the new value to it.
     * When this sentry is destroyed, the original value will be restored.
     *
     * @param v Pointer to the variable to manage (must not be null).
     * @param value The new value to assign to *v.
     */
    assignment_sentry_t(T *v, const T &value) :
            var(v), old_value(*var) {
        *var = value;
    }

    /**
     * @brief Destructor that restores the original value.
     *
     * If this sentry is managing a variable, restores its original value.
     * Safe to call even if the sentry was empty or already reset.
     */
    ~assignment_sentry_t() {
        reset();
    }

    /**
     * @brief Resets the sentry to manage a different variable with a new value.
     *
     * If the sentry was already managing a variable, its original value is
     * restored first. Then the sentry begins managing the new variable.
     *
     * @param v Pointer to the new variable to manage (must not be null).
     * @param value The new value to assign to *v.
     */
    void reset(T *v, const T &value) {
        reset();
        var = v;
        old_value = *var;
        *var = value;
    }

    /**
     * @brief Stops managing the current variable and restores its original value.
     *
     * If this sentry is managing a variable, restores its original value and
     * stops managing it. Subsequent calls to reset() have no effect until a
     * new variable is assigned.
     *
     * Safe to call multiple times.
     */
    void reset() {
        if (var != nullptr) {
            *var = old_value;
            var = nullptr;
        }
    }

private:
    T *var;           ///< Pointer to the variable being managed (nullptr if not managing)
    T old_value;      ///< The saved original value to restore
};

#endif  // ASSIGNMENT_SENTRY_HPP_
