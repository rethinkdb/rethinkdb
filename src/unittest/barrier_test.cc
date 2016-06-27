#include "unittest/gtest.hpp"

#include "arch/barrier.hpp"
#include "containers/scoped.hpp"

namespace unittest {

struct barrier_and_values_t {
    thread_barrier_t barrier;

    // Get them in opposite cache lines for good measure.
    static const int values_size = 512;
    char values[values_size];

    static const int low_index = 0;
    static const int high_index = 256;

    barrier_and_values_t() : barrier(2) {
        memset(values, 0, values_size);
    }
};

struct barrier_arg_t {
    barrier_and_values_t *bav;
    int number;
    int indexA;
    int indexB;
    bool error;
};

void *run_barrier_thread(void *v_arg) {
    barrier_arg_t *arg = static_cast<barrier_arg_t *>(v_arg);
    barrier_and_values_t *bav = arg->bav;
    int number = arg->number;
    int indexA = arg->indexA;
    int indexB = arg->indexB;

    for (int i = 0; i < 1000; ++i) {
        bav->values[indexA] = number;
        bav->barrier.wait();
        arg->error |= (bav->values[indexB] == number);
        bav->values[indexB] = number;
        bav->barrier.wait();
        arg->error |= (bav->values[indexA] == number);
    }

    return nullptr;
}


TEST(BarrierTest, OneThousandTest) {
    scoped_ptr_t<barrier_and_values_t> p(new barrier_and_values_t);
    barrier_arg_t bav1, bav2;
    bav1.bav = bav2.bav = p.get();
    bav1.number = 0;
    bav2.number = 1;
    bav1.indexA = bav2.indexB = barrier_and_values_t::low_index;
    bav2.indexA = bav1.indexB = barrier_and_values_t::high_index;
    bav1.error = bav2.error = false;

    pthread_t th1, th2;
    int res = pthread_create(&th1, nullptr, run_barrier_thread, &bav1);
    guarantee_xerr(res == 0, res, "pthread_create (for th1) failed");
    res = pthread_create(&th2, nullptr, run_barrier_thread, &bav2);
    guarantee_xerr(res == 0, res, "pthread_create (for th2) failed");

    void *dummy;
    res = pthread_join(th1, &dummy);
    guarantee_xerr(res == 0, res, "pthread_join (for th1) failed");
    res = pthread_join(th2, &dummy);
    guarantee_xerr(res == 0, res, "pthread_join (for th2) failed");

    ASSERT_FALSE(bav1.error);
    ASSERT_FALSE(bav2.error);
}





}  // namespace unittest
