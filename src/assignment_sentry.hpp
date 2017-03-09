#ifndef ASSIGNMENT_SENTRY_HPP_
#define ASSIGNMENT_SENTRY_HPP_

template <class T>
class assignment_sentry_t {
public:
    assignment_sentry_t() : var(nullptr), old_value() { }
    assignment_sentry_t(T *v, const T &value) :
            var(v), old_value(*var) {
        *var = value;
    }
    ~assignment_sentry_t() {
        reset();
    }
    void reset(T *v, const T &value) {
        reset();
        var = v;
        old_value = *var;
        *var = value;
    }
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
