#include "unittest/gtest.hpp"

#include "http/http.hpp"

namespace unittest {

TEST(ResourceTest, EmptyResource) {
    bool caught_exception = false;
    try {
        http_req_t::resource_t resource("");
        auto it = resource.begin();
        auto jt = resource.end();
    } catch (const std::invalid_argument &ex) {
        caught_exception = true;
    }

    ASSERT_TRUE(caught_exception);
};



}  // namespace unittest
