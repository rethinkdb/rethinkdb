//  (C) Copyright Antony Polukhin 2012.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//
// Testing variant performance rvalue copy/assign performance
//

#define BOOST_ERROR_CODE_HEADER_ONLY
#define BOOST_CHRONO_HEADER_ONLY
#include <boost/chrono.hpp>

#include <boost/variant.hpp>
#include <string>
#include <vector>

    struct scope {
        typedef boost::chrono::steady_clock test_clock;
        typedef boost::chrono::milliseconds duration_t;
        test_clock::time_point start_;
        const char* const message_;

        explicit scope(const char* const message)
            : start_(test_clock::now())
            , message_(message)
        {}

        ~scope() {
            std::cout << message_ << "   " << boost::chrono::duration_cast<duration_t>(test_clock::now() - start_) << std::endl;
        }
    };



static void do_test(bool do_count_cleanup_time = false) {
    BOOST_STATIC_CONSTANT(std::size_t, c_run_count = 5000000);
    typedef std::vector<char> str_t;
    typedef boost::variant<int, str_t, float> var_t;

    const char hello1_c[] = "hello long word";
    const str_t hello1(hello1_c, hello1_c + sizeof(hello1_c));

    const char hello2_c[] = "Helllloooooooooooooooooooooooooooooooo!!!!!!!!!!!!!";
    const str_t hello2(hello2_c, hello2_c + sizeof(hello2_c));

    if (do_count_cleanup_time) {
        std::cout << "#############################################\n";
        std::cout << "#############################################\n";    
        std::cout << "NOW TIMES WITH DATA DESTRUCTION\n";
        std::cout << "#############################################\n";
    }

    std::vector<var_t> data_from, data_to;
    data_from.resize(c_run_count, hello1);
    data_to.reserve(c_run_count);
    {
        scope sc("boost::variant(const variant&) copying speed");
        for (std::size_t i = 0; i < c_run_count; ++i) {
            data_to.push_back(data_from[i]); 
            
        }

        if (do_count_cleanup_time) {
            data_to.clear();
            data_from.clear();
        }
    }
    
    data_from.resize(c_run_count, hello1);
    data_to.clear();
    data_to.reserve(c_run_count);
    {
        scope sc("boost::variant(variant&&) moving speed");
        for (std::size_t i = 0; i < c_run_count; ++i) {
            data_to.push_back(std::move(data_from[i]));
        }

        if (do_count_cleanup_time) {
            data_to.clear();
            data_from.clear();
        }
    }

    std::cout << "#############################################\n";

    data_from.clear();
    data_from.resize(c_run_count, hello2);
    data_to.clear();
    data_to.resize(c_run_count, hello2);
    {
        scope sc("boost::variant=(const variant&) copying speed on same types");
        for (std::size_t i = 0; i < c_run_count; ++i) {
            data_to[i] = data_from[i]; 
        }

        if (do_count_cleanup_time) {
            data_to.clear();
            data_from.clear();
        }
    }

    data_from.resize(c_run_count, hello2);
    data_to.clear();
    data_to.resize(c_run_count, hello2);
    {
        scope sc("boost::variant=(variant&&) moving speed on same types");
        for (std::size_t i = 0; i < c_run_count; ++i) {
            data_to[i] = std::move(data_from[i]); 
        }

        if (do_count_cleanup_time) {
            data_to.clear();
            data_from.clear();
        }
    }

    std::cout << "#############################################\n";

    data_from.clear();
    data_from.resize(c_run_count, hello2);
    
    data_to.clear();
    data_to.resize(c_run_count, var_t(0));
    {
        scope sc("boost::variant=(const variant&) copying speed on different types");
        for (std::size_t i = 0; i < c_run_count; ++i) {
            data_to[i] = data_from[i]; 
        }

        if (do_count_cleanup_time) {
            data_to.clear();
            data_from.clear();
        }
    }

    data_from.resize(c_run_count, hello2);
    data_to.clear();
    data_to.resize(c_run_count, var_t(0));
    {
        scope sc("boost::variant=(variant&&) moving speed on different types");
        for (std::size_t i = 0; i < c_run_count; ++i) {
            data_to[i] = std::move(data_from[i]); 
        }

        if (do_count_cleanup_time) {
            data_to.clear();
            data_from.clear();
        }
    }

    std::cout << "#############################################\n";

    data_from.clear();
    data_from.resize(c_run_count, var_t(0));
    
    data_to.clear();
    data_to.resize(c_run_count, hello2);
    {
        scope sc("boost::variant=(const variant&) copying speed on different types II");
        for (std::size_t i = 0; i < c_run_count; ++i) {
            data_to[i] = data_from[i]; 
        }

        if (do_count_cleanup_time) {
            data_to.clear();
            data_from.clear();
        }
    }

    data_from.resize(c_run_count, var_t(0));
    data_to.clear();
    data_to.resize(c_run_count, hello2);
    {
        scope sc("boost::variant=(variant&&) moving speed on different types II");
        for (std::size_t i = 0; i < c_run_count; ++i) {
            data_to[i] = std::move(data_from[i]); 
        }

        if (do_count_cleanup_time) {
            data_to.clear();
            data_from.clear();
        }
    }


    std::cout << "#############################################\n";
    
    std::vector<str_t> s1(c_run_count, hello2);
    data_to.clear();
    data_to.resize(c_run_count, var_t(0));
    
    {
        scope sc("boost::variant=(const T&) copying speed");
        for (std::size_t i = 0; i < c_run_count; ++i) {
            data_to[i] = s1[i]; 
        }

        if (do_count_cleanup_time) {
            data_to.clear();
            s1.clear();
        }
    }

    std::vector<str_t> s2(c_run_count, hello2);
    data_to.clear();
    data_to.resize(c_run_count, var_t(0));
    {
        scope sc("boost::variant=(T&&) moving speed");
        for (std::size_t i = 0; i < c_run_count; ++i) {
            data_to[i] = std::move(s2[i]);
        }

        if (do_count_cleanup_time) {
            data_to.clear();
            s2.clear();
        }
    }
}


int main () {
    do_test(false);
    do_test(true);
}


