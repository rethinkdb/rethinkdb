#ifndef BOOST_THREAD_SYNC_BOUNDED_QUEUE_HPP
#define BOOST_THREAD_SYNC_BOUNDED_QUEUE_HPP

//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Vicente J. Botet Escriba 2013. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/thread for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/thread/detail/config.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/throw_exception.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{

  BOOST_SCOPED_ENUM_DECLARE_BEGIN(queue_op_status)
  { success = 0, empty, full, closed, busy }
  BOOST_SCOPED_ENUM_DECLARE_END(queue_op_status)

  struct no_block_tag{};
  BOOST_CONSTEXPR_OR_CONST no_block_tag no_block = {};

  struct sync_queue_is_closed : std::exception {};

  template <typename ValueType>
  class sync_bounded_queue
  {
  public:
    typedef ValueType value_type;
    typedef std::size_t size_type;

    // Constructors/Assignment/Destructors
    BOOST_THREAD_NO_COPYABLE(sync_bounded_queue)
    explicit sync_bounded_queue(size_type max_elems);
    template <typename Range>
    sync_bounded_queue(size_type max_elems, Range range);
    ~sync_bounded_queue();

    // Observers
    inline bool empty() const;
    inline bool full() const;
    inline size_type capacity() const;
    inline size_type size() const;
    inline bool closed() const;

    // Modifiers
    inline void close();

    inline void push(const value_type& x);
    inline void push(BOOST_THREAD_RV_REF(value_type) x);
    inline bool try_push(const value_type& x);
    inline bool try_push(BOOST_THREAD_RV_REF(value_type) x);
    inline bool try_push(no_block_tag, const value_type& x);
    inline bool try_push(no_block_tag, BOOST_THREAD_RV_REF(value_type) x);

    // Observers/Modifiers
    inline void pull(value_type&);
    inline void pull(ValueType& elem, bool & closed);
    // enable_if is_nothrow_copy_movable<value_type>
    inline value_type pull();
    inline shared_ptr<ValueType> ptr_pull();
    inline bool try_pull(value_type&);
    inline bool try_pull(no_block_tag,value_type&);
    inline shared_ptr<ValueType> try_pull();

  private:
    mutable mutex mtx_;
    condition_variable not_empty_;
    condition_variable not_full_;
    size_type waiting_full_;
    size_type waiting_empty_;
    value_type* data_;
    size_type in_;
    size_type out_;
    size_type capacity_;
    bool closed_;

    inline size_type inc(size_type idx) const BOOST_NOEXCEPT
    {
      return (idx + 1) % capacity_;
    }

    inline bool empty(unique_lock<mutex>& ) const BOOST_NOEXCEPT
    {
      return in_ == out_;
    }
    inline bool empty(lock_guard<mutex>& ) const BOOST_NOEXCEPT
    {
      return in_ == out_;
    }
    inline bool full(unique_lock<mutex>& ) const BOOST_NOEXCEPT
    {
      return (inc(in_) == out_);
    }
    inline bool full(lock_guard<mutex>& ) const BOOST_NOEXCEPT
    {
      return (inc(in_) == out_);
    }
    inline size_type capacity(lock_guard<mutex>& ) const BOOST_NOEXCEPT
    {
      return capacity_-1;
    }
    inline size_type size(lock_guard<mutex>& lk) const BOOST_NOEXCEPT
    {
      if (full(lk)) return capacity(lk);
      return ((out_+capacity(lk)-in_) % capacity(lk));
    }

    inline void throw_if_closed(unique_lock<mutex>&);

    inline bool try_pull(value_type& x, unique_lock<mutex>& lk);
    inline bool try_push(const value_type& x, unique_lock<mutex>& lk);
    inline bool try_push(BOOST_THREAD_RV_REF(value_type) x, unique_lock<mutex>& lk);
    inline shared_ptr<value_type> try_pull(unique_lock<mutex>& lk);

    inline void wait_until_not_empty(unique_lock<mutex>& lk);
    inline void wait_until_not_empty(unique_lock<mutex>& lk, bool&);
    inline size_type wait_until_not_full(unique_lock<mutex>& lk);
    inline size_type wait_until_not_full(unique_lock<mutex>& lk, bool&);


    inline void notify_not_empty_if_needed(unique_lock<mutex>& lk)
    {
      if (waiting_empty_ > 0)
      {
        --waiting_empty_;
        lk.unlock();
        not_empty_.notify_one();
      }
    }
    inline void notify_not_full_if_needed(unique_lock<mutex>& lk)
    {
      if (waiting_full_ > 0)
      {
        --waiting_full_;
        lk.unlock();
        not_full_.notify_one();
      }
    }

    inline void pull(value_type& elem, unique_lock<mutex>& lk)
    {
      elem = boost::move(data_[out_]);
      out_ = inc(out_);
      notify_not_full_if_needed(lk);
    }
    inline boost::shared_ptr<value_type> ptr_pull(unique_lock<mutex>& lk)
    {
      shared_ptr<value_type> res = make_shared<value_type>(boost::move(data_[out_]));
      out_ = inc(out_);
      notify_not_full_if_needed(lk);
      return res;
    }

    inline void set_in(size_type in, unique_lock<mutex>& lk)
    {
      in_ = in;
      notify_not_empty_if_needed(lk);
    }

    inline void push_at(const value_type& elem, size_type in_p_1, unique_lock<mutex>& lk)
    {
      data_[in_] = elem;
      set_in(in_p_1, lk);
    }

    inline void push_at(BOOST_THREAD_RV_REF(value_type) elem, size_type in_p_1, unique_lock<mutex>& lk)
    {
      data_[in_] = boost::move(elem);
      set_in(in_p_1, lk);
    }
  };

  template <typename ValueType>
  sync_bounded_queue<ValueType>::sync_bounded_queue(typename sync_bounded_queue<ValueType>::size_type max_elems) :
    waiting_full_(0), waiting_empty_(0), data_(new value_type[max_elems + 1]), in_(0), out_(0), capacity_(max_elems + 1),
        closed_(false)
  {
    BOOST_ASSERT_MSG(max_elems >= 1, "number of elements must be > 1");
  }

//  template <typename ValueType>
//  template <typename Range>
//  sync_bounded_queue<ValueType>::sync_bounded_queue(size_type max_elems, Range range) :
//    waiting_full_(0), waiting_empty_(0), data_(new value_type[max_elems + 1]), in_(0), out_(0), capacity_(max_elems + 1),
//        closed_(false)
//  {
//    BOOST_ASSERT_MSG(max_elems >= 1, "number of elements must be > 1");
//    BOOST_ASSERT_MSG(max_elems == size(range), "number of elements must match range's size");
//    try
//    {
//      typedef typename Range::iterator iterator_t;
//      iterator_t first = boost::begin(range);
//      iterator_t end = boost::end(range);
//      size_type in = 0;
//      for (iterator_t cur = first; cur != end; ++cur, ++in)
//      {
//        data_[in] = *cur;
//      }
//      set_in(in);
//    }
//    catch (...)
//    {
//      delete[] data_;
//    }
//  }

  template <typename ValueType>
  sync_bounded_queue<ValueType>::~sync_bounded_queue()
  {
    delete[] data_;
  }

  template <typename ValueType>
  void sync_bounded_queue<ValueType>::close()
  {
    {
      lock_guard<mutex> lk(mtx_);
      closed_ = true;
    }
    not_empty_.notify_all();
    not_full_.notify_all();
  }

  template <typename ValueType>
  bool sync_bounded_queue<ValueType>::closed() const
  {
    lock_guard<mutex> lk(mtx_);
    return closed_;
  }

  template <typename ValueType>
  bool sync_bounded_queue<ValueType>::empty() const
  {
    lock_guard<mutex> lk(mtx_);
    return empty(lk);
  }
  template <typename ValueType>
  bool sync_bounded_queue<ValueType>::full() const
  {
    lock_guard<mutex> lk(mtx_);
    return full(lk);
  }

  template <typename ValueType>
  typename sync_bounded_queue<ValueType>::size_type sync_bounded_queue<ValueType>::capacity() const
  {
    lock_guard<mutex> lk(mtx_);
    return capacity(lk);
  }

  template <typename ValueType>
  typename sync_bounded_queue<ValueType>::size_type sync_bounded_queue<ValueType>::size() const
  {
    lock_guard<mutex> lk(mtx_);
    return size(lk);
  }


  template <typename ValueType>
  bool sync_bounded_queue<ValueType>::try_pull(ValueType& elem, unique_lock<mutex>& lk)
  {
    if (empty(lk))
    {
      throw_if_closed(lk);
      return false;
    }
    pull(elem, lk);
    return true;
  }
  template <typename ValueType>
  shared_ptr<ValueType> sync_bounded_queue<ValueType>::try_pull(unique_lock<mutex>& lk)
  {
    if (empty(lk))
    {
      throw_if_closed(lk);
      return shared_ptr<ValueType>();
    }
    return ptr_pull(lk);
  }

  template <typename ValueType>
  bool sync_bounded_queue<ValueType>::try_pull(ValueType& elem)
  {
    try
    {
      unique_lock<mutex> lk(mtx_);
      return try_pull(elem, lk);
    }
    catch (...)
    {
      close();
      throw;
    }
  }

  template <typename ValueType>
  bool sync_bounded_queue<ValueType>::try_pull(no_block_tag,ValueType& elem)
  {
    try
    {
      unique_lock<mutex> lk(mtx_, try_to_lock);
      if (!lk.owns_lock())
      {
        return false;
      }
      return try_pull(elem, lk);
    }
    catch (...)
    {
      close();
      throw;
    }
  }
  template <typename ValueType>
  boost::shared_ptr<ValueType> sync_bounded_queue<ValueType>::try_pull()
  {
    try
    {
      unique_lock<mutex> lk(mtx_);
      return try_pull(lk);
    }
    catch (...)
    {
      close();
      throw;
    }
  }

  template <typename ValueType>
  void sync_bounded_queue<ValueType>::throw_if_closed(unique_lock<mutex>&)
  {
    if (closed_)
    {
      BOOST_THROW_EXCEPTION( sync_queue_is_closed() );
    }
  }

  template <typename ValueType>
  void sync_bounded_queue<ValueType>::wait_until_not_empty(unique_lock<mutex>& lk)
  {
    for (;;)
    {
      if (out_ != in_) break;
      throw_if_closed(lk);
      ++waiting_empty_;
      not_empty_.wait(lk);
    }
  }
  template <typename ValueType>
  void sync_bounded_queue<ValueType>::wait_until_not_empty(unique_lock<mutex>& lk, bool & closed)
  {
    for (;;)
    {
      if (out_ != in_) break;
      if (closed_) {closed=true; return;}
      ++waiting_empty_;
      not_empty_.wait(lk);
    }
  }

  template <typename ValueType>
  void sync_bounded_queue<ValueType>::pull(ValueType& elem)
  {
    try
    {
      unique_lock<mutex> lk(mtx_);
      wait_until_not_empty(lk);
      pull(elem, lk);
    }
    catch (...)
    {
      close();
      throw;
    }
  }
  template <typename ValueType>
  void sync_bounded_queue<ValueType>::pull(ValueType& elem, bool & closed)
  {
    try
    {
      unique_lock<mutex> lk(mtx_);
      wait_until_not_empty(lk, closed);
      if (closed) {return;}
      pull(elem, lk);
    }
    catch (...)
    {
      close();
      throw;
    }
  }

  // enable if ValueType is nothrow movable
  template <typename ValueType>
  ValueType sync_bounded_queue<ValueType>::pull()
  {
    try
    {
      value_type elem;
      pull(elem);
      return boost::move(elem);
    }
    catch (...)
    {
      close();
      throw;
    }
  }
  template <typename ValueType>
  boost::shared_ptr<ValueType> sync_bounded_queue<ValueType>::ptr_pull()
  {
    try
    {
      unique_lock<mutex> lk(mtx_);
      wait_until_not_empty(lk);
      return ptr_pull(lk);
    }
    catch (...)
    {
      close();
      throw;
    }
  }

  template <typename ValueType>
  bool sync_bounded_queue<ValueType>::try_push(const ValueType& elem, unique_lock<mutex>& lk)
  {
    throw_if_closed(lk);
    size_type in_p_1 = inc(in_);
    if (in_p_1 == out_)  // full()
    {
      return false;
    }
    push_at(elem, in_p_1, lk);
    return true;
  }

  template <typename ValueType>
  bool sync_bounded_queue<ValueType>::try_push(const ValueType& elem)
  {
    try
    {
      unique_lock<mutex> lk(mtx_);
      return try_push(elem, lk);
    }
    catch (...)
    {
      close();
      throw;
    }
  }

  template <typename ValueType>
  bool sync_bounded_queue<ValueType>::try_push(no_block_tag, const ValueType& elem)
  {
    try
    {
      unique_lock<mutex> lk(mtx_, try_to_lock);
      if (!lk.owns_lock()) return false;
      return try_push(elem, lk);
    }
    catch (...)
    {
      close();
      throw;
    }
  }


  template <typename ValueType>
  typename sync_bounded_queue<ValueType>::size_type sync_bounded_queue<ValueType>::wait_until_not_full(unique_lock<mutex>& lk)
  {
    for (;;)
    {
      throw_if_closed(lk);
      size_type in_p_1 = inc(in_);
      if (in_p_1 != out_) // ! full()
      {
        return in_p_1;
      }
      ++waiting_full_;
      not_full_.wait(lk);
    }
  }

  template <typename ValueType>
  void sync_bounded_queue<ValueType>::push(const ValueType& elem)
  {
    try
    {
      unique_lock<mutex> lk(mtx_);
      push_at(elem, wait_until_not_full(lk), lk);
    }
    catch (...)
    {
      close();
      throw;
    }
  }

  template <typename ValueType>
  bool sync_bounded_queue<ValueType>::try_push(BOOST_THREAD_RV_REF(ValueType) elem, unique_lock<mutex>& lk)
  {
    throw_if_closed(lk);
    size_type in_p_1 = inc(in_);
    if (in_p_1 == out_) // full()
    {
      return false;
    }
    push_at(boost::move(elem), in_p_1, lk);
    return true;
  }

  template <typename ValueType>
  bool sync_bounded_queue<ValueType>::try_push(BOOST_THREAD_RV_REF(ValueType) elem)
  {
    try
    {
      unique_lock<mutex> lk(mtx_);
      return try_push(elem, lk);
    }
    catch (...)
    {
      close();
      throw;
    }
  }

  template <typename ValueType>
  bool sync_bounded_queue<ValueType>::try_push(no_block_tag, BOOST_THREAD_RV_REF(ValueType) elem)
  {
    try
    {
      unique_lock<mutex> lk(mtx_, try_to_lock);
      if (!lk.owns_lock())
      {
        return false;
      }
      return try_push(elem, lk);
    }
    catch (...)
    {
      close();
      throw;
    }
  }

  template <typename ValueType>
  void sync_bounded_queue<ValueType>::push(BOOST_THREAD_RV_REF(ValueType) elem)
  {
    try
    {
      unique_lock<mutex> lk(mtx_);
      push_at(elem, wait_until_not_full(lk), lk);
    }
    catch (...)
    {
      close();
      throw;
    }
  }

  template <typename ValueType>
  sync_bounded_queue<ValueType>& operator<<(sync_bounded_queue<ValueType>& sbq, BOOST_THREAD_RV_REF(ValueType) elem)
  {
    sbq.push(boost::forward<ValueType>(elem));
    return sbq;
  }

  template <typename ValueType>
  sync_bounded_queue<ValueType>& operator<<(sync_bounded_queue<ValueType>& sbq, ValueType const&elem)
  {
    sbq.push(elem);
    return sbq;
  }

  template <typename ValueType>
  sync_bounded_queue<ValueType>& operator>>(sync_bounded_queue<ValueType>& sbq, ValueType &elem)
  {
    sbq.pull(elem);
    return sbq;
  }

}

#include <boost/config/abi_suffix.hpp>

#endif
