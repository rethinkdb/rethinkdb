
// Copyright 2008-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or move at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_UNORDERED_TEST_HELPERS_COUNT_HEAD)
#define BOOST_UNORDERED_TEST_HELPERS_COUNT_HEAD

#include <iostream>

namespace test {
    struct object_count {
        int instances;
        int constructions;

        object_count() : instances(0), constructions(0) {}
        void reset() { *this = object_count(); }

        void construct() {
            ++instances;
            ++constructions;
        }

        void destruct() {
            if(instances == 0) {
                BOOST_ERROR("Unbalanced constructions.");
            }
            else {
                --instances;
            }
        }

        bool operator==(object_count const& x) const {
            return instances == x.instances &&
                constructions == x.constructions;
        }

        bool operator!=(object_count const& x) const {
            return !(*this == x);
        }
        
        friend std::ostream& operator<<(std::ostream& out,
            object_count const& c)
        {
            out
                << "[instances: "
                << c.instances
                << ", constructions: "
                << c.constructions
                << "]";
            return out;
        }
    };

    // This won't be a problem as I'm only using a single compile unit
    // in each test (this is actually require by the minimal test
    // framework).
    // 
    // boostinspect:nounnamed
    namespace {
        object_count global_object_count;
    }
    
    struct counted_object
    {
        counted_object() { global_object_count.construct(); }
        counted_object(counted_object const&) { global_object_count.construct(); }
        ~counted_object() { global_object_count.destruct(); }
    };

    struct check_instances {
        int instances;
        
        check_instances() : instances(global_object_count.instances) {}
        ~check_instances() { BOOST_TEST(global_object_count.instances == instances); }
    };
}

#endif
