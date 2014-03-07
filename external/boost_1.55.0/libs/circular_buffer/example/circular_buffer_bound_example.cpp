// Copyright 2003-2008 Jan Gaspar.
// Copyright 2013 Paul A. Bristow. Added some Quickbook snippet markers.

// Distributed under the Boost Software License, Version 1.0.
// (See the accompanying file LICENSE_1_0.txt
// or a copy at <http://www.boost.org/LICENSE_1_0.txt>.)

//[circular_buffer_bound_example_1
/*`
This example shows how the `circular_buffer` can be utilized
as an underlying container of the bounded buffer.
*/

#include <boost/circular_buffer.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/thread.hpp>
#include <boost/call_traits.hpp>
#include <boost/bind.hpp>

#include <boost/timer/timer.hpp> // for auto_cpu_timer

template <class T>
class bounded_buffer
{
public:

  typedef boost::circular_buffer<T> container_type;
  typedef typename container_type::size_type size_type;
  typedef typename container_type::value_type value_type;
  typedef typename boost::call_traits<value_type>::param_type param_type;

  explicit bounded_buffer(size_type capacity) : m_unread(0), m_container(capacity) {}

  void push_front(typename boost::call_traits<value_type>::param_type item)
  { // `param_type` represents the "best" way to pass a parameter of type `value_type` to a method.

      boost::mutex::scoped_lock lock(m_mutex);
      m_not_full.wait(lock, boost::bind(&bounded_buffer<value_type>::is_not_full, this));
      m_container.push_front(item);
      ++m_unread;
      lock.unlock();
      m_not_empty.notify_one();
  }

  void pop_back(value_type* pItem) {
      boost::mutex::scoped_lock lock(m_mutex);
      m_not_empty.wait(lock, boost::bind(&bounded_buffer<value_type>::is_not_empty, this));
      *pItem = m_container[--m_unread];
      lock.unlock();
      m_not_full.notify_one();
  }

private:
  bounded_buffer(const bounded_buffer&);              // Disabled copy constructor.
  bounded_buffer& operator = (const bounded_buffer&); // Disabled assign operator.

  bool is_not_empty() const { return m_unread > 0; }
  bool is_not_full() const { return m_unread < m_container.capacity(); }

  size_type m_unread;
  container_type m_container;
  boost::mutex m_mutex;
  boost::condition m_not_empty;
  boost::condition m_not_full;
}; //

//] [/circular_buffer_bound_example_1]

const unsigned long queue_size     = 1000L;
const unsigned long total_elements = queue_size * 1000L;

//[circular_buffer_bound_example_2]
/*`To demonstrate, create two classes to exercise the buffer.

The producer class fills the buffer with elements.

The consumer class consumes the buffer contents.

*/

template<class Buffer>
class Producer
{

    typedef typename Buffer::value_type value_type;
    Buffer* m_container;

public:
    Producer(Buffer* buffer) : m_container(buffer)
    {}

    void operator()()
    {
        for (unsigned long i = 0L; i < total_elements; ++i)
        {
            m_container->push_front(value_type());
        }
    }
};

template<class Buffer>
class Consumer
{

    typedef typename Buffer::value_type value_type;
    Buffer* m_container;
    value_type m_item;

public:
    Consumer(Buffer* buffer) : m_container(buffer)
    {}

    void operator()()
    {
        for (unsigned long i = 0L; i < total_elements; ++i)
        {
            m_container->pop_back(&m_item);
        }
    }
};

/*`Create a first-int first-out test of the bound_buffer.
Include a call to boost::progress_timer

[@http://www.boost.org/doc/libs/1_53_0/libs/timer/doc/cpu_timers.html CPU timer]

*/
template<class Buffer>
void fifo_test(Buffer* buffer)
{
    // Start of timing.
    boost::timer::auto_cpu_timer progress;

    // Initialize the buffer with some values before launching producer and consumer threads.
    for (unsigned long i = queue_size / 2L; i > 0; --i)
    {
#if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x581))
        buffer->push_front(Buffer::value_type());
#else
        buffer->push_front(BOOST_DEDUCED_TYPENAME Buffer::value_type());
#endif
    }

    // Construct the threads.
    Consumer<Buffer> consumer(buffer);
    Producer<Buffer> producer(buffer);

    // Start the threads.
    boost::thread consume(consumer);
    boost::thread produce(producer);

    // Wait for completion.
    consume.join();
    produce.join();

    // End of timing.
    // destructor of boost::timer::auto_cpu_timer will output the time to std::cout.

}
//] [/circular_buffer_bound_example_2]


int main()
{
//[circular_buffer_bound_example_3]
    //`Construct a bounded_buffer to hold the chosen type, here int.
    bounded_buffer<int> bb_int(queue_size);
    std::cout << "Testing bounded_buffer<int> ";

    //`Start the fifo test.
    fifo_test(&bb_int);
   //` destructor of boost::timer::auto_cpu_timer will output the time to std::cout

//] [/circular_buffer_bound_example_3]

return 0;
} // int main()

/*

//[circular_buffer_bound_output

  Description: Autorun "J:\Cpp\Misc\Debug\circular_buffer_bound_example.exe" 

  Testing bounded_buffer<int>  15.010692s wall, 9.188459s user + 7.207246s system = 16.395705s CPU (109.2%)

//] [/circular_buffer_bound_output]
*/


