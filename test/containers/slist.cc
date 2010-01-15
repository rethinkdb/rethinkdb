
#include <retest.hpp>
#include "containers/slist.hpp"

void test_slist() {
    typedef slist_node_t<void> list_t;
    list_t *hd;

    // Populate the list
    hd = cons((void*)0, (list_t*)NULL);
    hd = cons((void*)1, hd);
    hd = cons((void*)2, hd);

    // Go through the list
    list_t *tmp = hd;
    for(int i = 2; i >= 0; i--) {
        assert_eq(i, (long)head(tmp));
        tmp = tail(tmp);
    }

    // Free the list
    while(hd) {
        hd = free_node(hd);
    }
}
