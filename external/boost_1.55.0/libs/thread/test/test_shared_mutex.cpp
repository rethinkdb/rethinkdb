// (C) Copyright 2006-7 Anthony Williams
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_VERSION 2
#define BOOST_THREAD_PROVIDES_INTERRUPTIONS

#include <boost/test/unit_test.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>
#include "./util.inl"
#include "./shared_mutex_locking_thread.hpp"

#define CHECK_LOCKED_VALUE_EQUAL(mutex_name,value,expected_value)    \
    {                                                                \
        boost::unique_lock<boost::mutex> lock(mutex_name);                  \
        BOOST_CHECK_EQUAL(value,expected_value);                     \
    }

void test_multiple_readers()
{
  std::cout << __LINE__ << std::endl;
    unsigned const number_of_threads=10;

    boost::thread_group pool;

    boost::shared_mutex rw_mutex;
    unsigned unblocked_count=0;
    unsigned simultaneous_running_count=0;
    unsigned max_simultaneous_running=0;
    boost::mutex unblocked_count_mutex;
    boost::condition_variable unblocked_condition;
    boost::mutex finish_mutex;
    boost::unique_lock<boost::mutex> finish_lock(finish_mutex);

    try
    {
        for(unsigned i=0;i<number_of_threads;++i)
        {
            pool.create_thread(locking_thread<boost::shared_lock<boost::shared_mutex> >(rw_mutex,unblocked_count,unblocked_count_mutex,unblocked_condition,
                                                                                        finish_mutex,simultaneous_running_count,max_simultaneous_running));
        }

        {
            boost::unique_lock<boost::mutex> lk(unblocked_count_mutex);
            while(unblocked_count<number_of_threads)
            {
                unblocked_condition.wait(lk);
            }
        }

        CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,unblocked_count,number_of_threads);

        finish_lock.unlock();

        pool.join_all();
    }
    catch(...)
    {
        pool.interrupt_all();
        pool.join_all();
        throw;
    }

    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,max_simultaneous_running,number_of_threads);
}

void test_only_one_writer_permitted()
{
  std::cout << __LINE__ << std::endl;
    unsigned const number_of_threads=10;

    boost::thread_group pool;

    boost::shared_mutex rw_mutex;
    unsigned unblocked_count=0;
    unsigned simultaneous_running_count=0;
    unsigned max_simultaneous_running=0;
    boost::mutex unblocked_count_mutex;
    boost::condition_variable unblocked_condition;
    boost::mutex finish_mutex;
    boost::unique_lock<boost::mutex> finish_lock(finish_mutex);

    try
    {
        for(unsigned i=0;i<number_of_threads;++i)
        {
            pool.create_thread(locking_thread<boost::unique_lock<boost::shared_mutex> >(rw_mutex,unblocked_count,unblocked_count_mutex,unblocked_condition,
                                                                                        finish_mutex,simultaneous_running_count,max_simultaneous_running));
        }

        boost::thread::sleep(delay(2));

        CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,unblocked_count,1U);

        finish_lock.unlock();

        pool.join_all();
    }
    catch(...)
    {
        pool.interrupt_all();
        pool.join_all();
        throw;
    }

    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,unblocked_count,number_of_threads);
    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,max_simultaneous_running,1u);
}

void test_reader_blocks_writer()
{
  std::cout << __LINE__ << std::endl;
    boost::thread_group pool;

    boost::shared_mutex rw_mutex;
    unsigned unblocked_count=0;
    unsigned simultaneous_running_count=0;
    unsigned max_simultaneous_running=0;
    boost::mutex unblocked_count_mutex;
    boost::condition_variable unblocked_condition;
    boost::mutex finish_mutex;
    boost::unique_lock<boost::mutex> finish_lock(finish_mutex);

    try
    {

        pool.create_thread(locking_thread<boost::shared_lock<boost::shared_mutex> >(rw_mutex,unblocked_count,unblocked_count_mutex,unblocked_condition,
                                                                                    finish_mutex,simultaneous_running_count,max_simultaneous_running));
        {
            boost::unique_lock<boost::mutex> lk(unblocked_count_mutex);
            while(unblocked_count<1)
            {
                unblocked_condition.wait(lk);
            }
        }
        CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,unblocked_count,1U);
        pool.create_thread(locking_thread<boost::unique_lock<boost::shared_mutex> >(rw_mutex,unblocked_count,unblocked_count_mutex,unblocked_condition,
                                                                                    finish_mutex,simultaneous_running_count,max_simultaneous_running));
        boost::thread::sleep(delay(1));
        CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,unblocked_count,1U);

        finish_lock.unlock();

        pool.join_all();
    }
    catch(...)
    {
        pool.interrupt_all();
        pool.join_all();
        throw;
    }

    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,unblocked_count,2U);
    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,max_simultaneous_running,1u);
}

void test_unlocking_writer_unblocks_all_readers()
{
  std::cout << __LINE__ << std::endl;
    boost::thread_group pool;

    boost::shared_mutex rw_mutex;
    boost::unique_lock<boost::shared_mutex>  write_lock(rw_mutex);
    unsigned unblocked_count=0;
    unsigned simultaneous_running_count=0;
    unsigned max_simultaneous_running=0;
    boost::mutex unblocked_count_mutex;
    boost::condition_variable unblocked_condition;
    boost::mutex finish_mutex;
    boost::unique_lock<boost::mutex> finish_lock(finish_mutex);

    unsigned const reader_count=10;

    try
    {
        for(unsigned i=0;i<reader_count;++i)
        {
            pool.create_thread(locking_thread<boost::shared_lock<boost::shared_mutex> >(rw_mutex,unblocked_count,unblocked_count_mutex,unblocked_condition,
                                                                                        finish_mutex,simultaneous_running_count,max_simultaneous_running));
        }
        boost::thread::sleep(delay(1));
        CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,unblocked_count,0U);

        write_lock.unlock();

        {
            boost::unique_lock<boost::mutex> lk(unblocked_count_mutex);
            while(unblocked_count<reader_count)
            {
                unblocked_condition.wait(lk);
            }
        }

        CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,unblocked_count,reader_count);

        finish_lock.unlock();
        pool.join_all();
    }
    catch(...)
    {
        pool.interrupt_all();
        pool.join_all();
        throw;
    }

    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,max_simultaneous_running,reader_count);
}

void test_unlocking_last_reader_only_unblocks_one_writer()
{
  std::cout << __LINE__ << std::endl;
    boost::thread_group pool;

    boost::shared_mutex rw_mutex;
    unsigned unblocked_count=0;
    unsigned simultaneous_running_readers=0;
    unsigned max_simultaneous_readers=0;
    unsigned simultaneous_running_writers=0;
    unsigned max_simultaneous_writers=0;
    boost::mutex unblocked_count_mutex;
    boost::condition_variable unblocked_condition;
    boost::mutex finish_reading_mutex;
    boost::unique_lock<boost::mutex> finish_reading_lock(finish_reading_mutex);
    boost::mutex finish_writing_mutex;
    boost::unique_lock<boost::mutex> finish_writing_lock(finish_writing_mutex);

    unsigned const reader_count=10;
    unsigned const writer_count=10;

    try
    {
        for(unsigned i=0;i<reader_count;++i)
        {
            pool.create_thread(locking_thread<boost::shared_lock<boost::shared_mutex> >(rw_mutex,unblocked_count,unblocked_count_mutex,unblocked_condition,
                                                                                        finish_reading_mutex,simultaneous_running_readers,max_simultaneous_readers));
        }
        boost::thread::sleep(delay(1));
        for(unsigned i=0;i<writer_count;++i)
        {
            pool.create_thread(locking_thread<boost::unique_lock<boost::shared_mutex> >(rw_mutex,unblocked_count,unblocked_count_mutex,unblocked_condition,
                                                                                        finish_writing_mutex,simultaneous_running_writers,max_simultaneous_writers));
        }
        {
            boost::unique_lock<boost::mutex> lk(unblocked_count_mutex);
            while(unblocked_count<reader_count)
            {
                unblocked_condition.wait(lk);
            }
        }
        boost::thread::sleep(delay(1));
        CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,unblocked_count,reader_count);

        finish_reading_lock.unlock();

        {
            boost::unique_lock<boost::mutex> lk(unblocked_count_mutex);
            while(unblocked_count<(reader_count+1))
            {
                unblocked_condition.wait(lk);
            }
        }
        CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,unblocked_count,reader_count+1);

        finish_writing_lock.unlock();
        pool.join_all();
    }
    catch(...)
    {
        pool.interrupt_all();
        pool.join_all();
        throw;
    }

    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,unblocked_count,reader_count+writer_count);
    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,max_simultaneous_readers,reader_count);
    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,max_simultaneous_writers,1u);
}

boost::unit_test::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test::test_suite* test =
        BOOST_TEST_SUITE("Boost.Threads: shared_mutex test suite");

    test->add(BOOST_TEST_CASE(&test_multiple_readers));
    test->add(BOOST_TEST_CASE(&test_only_one_writer_permitted));
    test->add(BOOST_TEST_CASE(&test_reader_blocks_writer));
    test->add(BOOST_TEST_CASE(&test_unlocking_writer_unblocks_all_readers));
    test->add(BOOST_TEST_CASE(&test_unlocking_last_reader_only_unblocks_one_writer));

    return test;
}


