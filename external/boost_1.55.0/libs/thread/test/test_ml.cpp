// Copyright (C) 2010 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config.hpp>
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

#include <boost/detail/lightweight_test.hpp>
#include <boost/thread/future.hpp>
#include <boost/utility/result_of.hpp>
#include <functional>

struct async_func {
    virtual ~async_func() { }
    virtual void run() =0;
    };

template <typename Ret>
class async_func_pt : public async_func {
    boost::packaged_task<Ret> f;
public:
    void run() {
      std::cout << __FILE__ << ":" << __LINE__ << std::endl;
f(); }
    async_func_pt (boost::packaged_task<Ret>&& f) : f(boost::move(f)) {}
    ~async_func_pt() { }
    boost::unique_future<Ret> get_future()  { return f.get_future(); }
    };

void async_core (async_func* p) {
  std::cout << __FILE__ << ":" << __LINE__ << std::endl;
  p->run();
}




template <typename F>
boost::unique_future<typename boost::result_of< F() >::type>
async (F&& f)
 {
  std::cout << __FILE__ << ":" << __LINE__ << std::endl;
 typedef typename boost::result_of< F() >::type RetType;
 std::cout << __FILE__ << ":" << __LINE__ << std::endl;
 async_func_pt<RetType>* p= new  async_func_pt<RetType> (boost::packaged_task<RetType>(boost::forward<F>(f)));
 std::cout << __FILE__ << ":" << __LINE__ << std::endl;
 boost::unique_future<RetType> future_result= p->get_future();
 std::cout << __FILE__ << ":" << __LINE__ << std::endl;
 async_core (p);
 std::cout << __FILE__ << ":" << __LINE__ << std::endl;
 return boost::move(future_result);
 }

template <typename F, typename A1>
boost::unique_future<typename boost::result_of< F(A1) >::type>
async (F&& f, A1&& a1)
 {
  std::cout << __FILE__ << ":" << __LINE__ << std::endl;
// This should be all it needs.  But get a funny error deep inside Boost.
// problem overloading with && ?
 return async (boost::bind(f,a1));
 }


int calculate_the_answer_to_life_the_universe_and_everything()
{
    return 42;
}


size_t foo (const std::string& s)
 {
 return s.size();
 }



void test1()
{
// this one works
    // most fundimental form:
  std::cout << __FILE__ << ":" << __LINE__ << std::endl;
    boost::unique_future<int> fi= async (&calculate_the_answer_to_life_the_universe_and_everything);
    int i= fi.get();
    BOOST_TEST (i== 42);


// This one chokes at compile time
    std::cout << __FILE__ << ":" << __LINE__ << std::endl;
    boost::unique_future<size_t> fut_1= async (&foo, "Life");

    BOOST_TEST (fut_1.get()== 4);
}

int main()
{
  test1();
  return boost::report_errors();

}

#else
int main()
{
  return 0;
}

#endif
/*
 *
 "/Users/viboes/clang/llvmCore-3.0-rc1.install/bin/clang++"
 -o "../../../bin.v2/libs/thread/test/test_ml.test/clang-darwin-3.0x/debug/threading-multi/test_ml"
 "../../../bin.v2/libs/thread/test/test_ml.test/clang-darwin-3.0x/debug/threading-multi/test_ml.o"
 "../../../bin.v2/libs/test/build/clang-darwin-3.0x/debug/link-static/threading-multi/libboost_unit_test_framework.a"
 "../../../bin.v2/libs/thread/build/clang-darwin-3.0x/debug/threading-multi/libboost_thread.dylib"
 "../../../bin.v2/libs/chrono/build/clang-darwin-3.0x/debug/threading-multi/libboost_chrono.dylib"
 "../../../bin.v2/libs/system/build/clang-darwin-3.0x/debug/threading-multi/libboost_system.dylib"    -g
*/


/*
 *
 * #include <boost/test/unit_test.hpp>
#include <boost/thread/future.hpp>
#include <boost/utility/result_of.hpp>
#include <functional>

struct async_func {
    virtual ~async_func() { }
    virtual void run() =0;
    };

template <typename Ret>
class async_func_pt : public async_func {
    boost::packaged_task<Ret> f;
public:
    void run() override { f(); }
    async_func_pt (boost::packaged_task<Ret>&& f) : f(std::move(f)) {}
    ~async_func_pt() { }
    boost::unique_future<Ret> get_future()  { return f.get_future(); }
    };

void async_core (async_func* p);




template <typename F>
boost::unique_future<typename boost::result_of< F() >::type>
async (F&& f)
 {
 typedef typename boost::result_of< F() >::type RetType;
 async_func_pt<RetType>* p= new  async_func_pt<RetType> (boost::packaged_task<RetType>(f));
 boost::unique_future<RetType> future_result= p->get_future();
 async_core (p);
 return std::move(future_result);
 }

template <typename F, typename A1>
boost::unique_future<typename boost::result_of< F(A1) >::type>
async (F&& f, A1&& a1)
 {
// This should be all it needs.  But get a funny error deep inside Boost.
// problem overloading with && ?
 return async (std::tr1::bind(f,a1));
 }

BOOST_AUTO_TEST_SUITE(thread_pool_tests)

int calculate_the_answer_to_life_the_universe_and_everything()
{
    return 42;
}


size_t foo (const std::string& s)
 {
 return s.size();
 }



BOOST_AUTO_TEST_CASE( async_test )
{
// this one works
    // most fundimental form:
    boost::unique_future<int> fi= async (&calculate_the_answer_to_life_the_universe_and_everything);
    int i= fi.get();
    BOOST_CHECK_EQUAL (i, 42);


// This one chokes at compile time
    boost::unique_future<size_t> fut_1= async (&foo, "Life");

    BOOST_CHECK_EQUAL (fut_1.get(), 4);
}


BOOST_AUTO_TEST_SUITE_END()
 */
