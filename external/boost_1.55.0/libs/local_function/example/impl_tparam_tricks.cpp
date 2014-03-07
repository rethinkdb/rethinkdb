
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

//[impl_tparam_tricks
#include <boost/detail/lightweight_test.hpp>
#include <vector>
#include <algorithm>

// Casting functor trick.
struct casting_func {
    explicit casting_func(void* obj, void (*call)(void*, const int&))
            : obj_(obj), call_(call) {}
    // Unfortunately, function pointer call is not inlined.
    inline void operator()(const int& num) { call_(obj_, num); }
private:
    void* obj_;
    void (*call_)(void*, const int&);
};

// Virtual functor trick.
struct virtual_func {
    struct interface {
        // Unfortunately, virtual function call is not inlined.
        inline virtual void operator()(const int&) {}
    };
    explicit virtual_func(interface& func): func_(&func) {}
    inline void operator()(const int& num) { (*func_)(num); }
private:
    interface* func_;
};

int main(void) {
    int sum = 0, factor = 10;

    // Local class for local function.
    struct local_add : virtual_func::interface {
        explicit local_add(int& _sum, const int& _factor)
                : sum_(_sum), factor_(_factor) {}
        inline void operator()(const int& num) {
            body(sum_, factor_, num);
        }
        inline static void call(void* obj, const int& num) {
            local_add* self = static_cast<local_add*>(obj);
            self->body(self->sum_, self->factor_, num);
        }
    private:
        int& sum_;
        const int& factor_;
        inline void body(int& sum, const int& factor, const int& num) {
            sum += factor * num;
        }
    } add_local(sum, factor);
    casting_func add_casting(&add_local, &local_add::call);
    virtual_func add_virtual(add_local);

    std::vector<int> v(10);
    std::fill(v.begin(), v.end(), 1);
    
    // std::for_each(v.begin(), v.end(), add_local); // Error but OK on C++11.
    std::for_each(v.begin(), v.end(), add_casting); // OK.
    std::for_each(v.begin(), v.end(), add_virtual); // OK.

    BOOST_TEST(sum == 200);
    return boost::report_errors();
}
//]

