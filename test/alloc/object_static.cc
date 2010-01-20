
#include "retest.hpp"
#include "alloc/malloc.hpp"
#include "alloc/object_static.hpp"

struct object1_t {
    int foo;
};

struct object2_t {
    int bar, baz;
};

struct object3_t {
    object3_t(int *_bak, int _value) : bak(_bak), value(_value) {}
    ~object3_t() {
        *bak = value;
    }
    int *bak;
    int value;
};

// Adapter because object_static_alloc expects super_alloc's constructor to take a parameter
template<typename super_alloc_t>
struct sized_malloc_adapter_t : public super_alloc_t {
    sized_malloc_adapter_t(size_t size) {
        // Size ignored
    }
};

void test_object_alloc() {
    object_static_alloc_t<sized_malloc_adapter_t<malloc_alloc_t>,
        object1_t, object2_t> pool;

    object1_t *obj1 = pool.malloc<object1_t>();
    obj1->foo = 1;
    
    object2_t *obj2 = pool.malloc<object2_t>();
    obj2->bar = 2;
    obj2->baz = 3;

    assert_eq(obj1->foo, 1);
    pool.free(obj1);
    
    assert_eq(obj2->bar, 2);
    assert_eq(obj2->baz, 3);
    pool.free(obj2);
}

void test_constructor_destructor() {
    object_static_alloc_t<sized_malloc_adapter_t<malloc_alloc_t>, object3_t> pool;

    int res = 0;
    object3_t *obj = pool.malloc<object3_t>(&res, 123);
    assert_eq(res, 0);

    pool.free(obj);
    assert_eq(res, 123);
}
