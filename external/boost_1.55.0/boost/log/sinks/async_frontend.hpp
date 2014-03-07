/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   async_frontend.hpp
 * \author Andrey Semashev
 * \date   14.07.2009
 *
 * The header contains implementation of asynchronous sink frontend.
 */

#ifndef BOOST_LOG_SINKS_ASYNC_FRONTEND_HPP_INCLUDED_
#define BOOST_LOG_SINKS_ASYNC_FRONTEND_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if defined(BOOST_LOG_NO_THREADS)
#error Boost.Log: Asynchronous sink frontend is only supported in multithreaded environment
#endif

#include <boost/bind.hpp>
#include <boost/static_assert.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/log/exceptions.hpp>
#include <boost/log/detail/locking_ptr.hpp>
#include <boost/log/detail/parameter_tools.hpp>
#include <boost/log/core/record_view.hpp>
#include <boost/log/sinks/basic_sink_frontend.hpp>
#include <boost/log/sinks/frontend_requirements.hpp>
#include <boost/log/sinks/unbounded_fifo_queue.hpp>
#include <boost/log/keywords/start_thread.hpp>
#include <boost/log/detail/header.hpp>

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace sinks {

#ifndef BOOST_LOG_DOXYGEN_PASS

#define BOOST_LOG_SINK_CTOR_FORWARD_INTERNAL(z, n, types)\
    template< BOOST_PP_ENUM_PARAMS(n, typename T) >\
    explicit asynchronous_sink(BOOST_PP_ENUM_BINARY_PARAMS(n, T, const& arg)) :\
        base_type(true),\
        queue_base_type((BOOST_PP_ENUM_PARAMS(n, arg))),\
        m_pBackend(boost::make_shared< sink_backend_type >(BOOST_PP_ENUM_PARAMS(n, arg))),\
        m_StopRequested(false),\
        m_FlushRequested(false)\
    {\
        if ((BOOST_PP_ENUM_PARAMS(n, arg))[keywords::start_thread | true])\
            start_feeding_thread();\
    }\
    template< BOOST_PP_ENUM_PARAMS(n, typename T) >\
    explicit asynchronous_sink(shared_ptr< sink_backend_type > const& backend, BOOST_PP_ENUM_BINARY_PARAMS(n, T, const& arg)) :\
        base_type(true),\
        queue_base_type((BOOST_PP_ENUM_PARAMS(n, arg))),\
        m_pBackend(backend),\
        m_StopRequested(false),\
        m_FlushRequested(false)\
    {\
        if ((BOOST_PP_ENUM_PARAMS(n, arg))[keywords::start_thread | true])\
            start_feeding_thread();\
    }

#endif // BOOST_LOG_DOXYGEN_PASS

/*!
 * \brief Asynchronous logging sink frontend
 *
 * The frontend starts a separate thread on construction. All logging records are passed
 * to the backend in this dedicated thread only.
 */
template< typename SinkBackendT, typename QueueingStrategyT = unbounded_fifo_queue >
class asynchronous_sink :
    public aux::make_sink_frontend_base< SinkBackendT >::type,
    private boost::log::aux::locking_ptr_counter_base,
    public QueueingStrategyT
{
    typedef typename aux::make_sink_frontend_base< SinkBackendT >::type base_type;
    typedef QueueingStrategyT queue_base_type;

private:
    //! Backend synchronization mutex type
    typedef boost::mutex backend_mutex_type;
    //! Frontend synchronization mutex type
    typedef typename base_type::mutex_type frontend_mutex_type;

    //! A scope guard that implements thread ID management
    class scoped_thread_id
    {
    private:
        frontend_mutex_type& m_Mutex;
        condition_variable_any& m_Cond;
        thread::id& m_ThreadID;
        bool volatile& m_StopRequested;

    public:
        //! Initializing constructor
        scoped_thread_id(frontend_mutex_type& mut, condition_variable_any& cond, thread::id& tid, bool volatile& sr)
            : m_Mutex(mut), m_Cond(cond), m_ThreadID(tid), m_StopRequested(sr)
        {
            lock_guard< frontend_mutex_type > lock(m_Mutex);
            if (m_ThreadID != thread::id())
                BOOST_LOG_THROW_DESCR(unexpected_call, "Asynchronous sink frontend already runs a record feeding thread");
            m_ThreadID = this_thread::get_id();
        }
        //! Initializing constructor
        scoped_thread_id(unique_lock< frontend_mutex_type >& l, condition_variable_any& cond, thread::id& tid, bool volatile& sr)
            : m_Mutex(*l.mutex()), m_Cond(cond), m_ThreadID(tid), m_StopRequested(sr)
        {
            unique_lock< frontend_mutex_type > lock(move(l));
            if (m_ThreadID != thread::id())
                BOOST_LOG_THROW_DESCR(unexpected_call, "Asynchronous sink frontend already runs a record feeding thread");
            m_ThreadID = this_thread::get_id();
        }
        //! Destructor
        ~scoped_thread_id()
        {
            try
            {
                lock_guard< frontend_mutex_type > lock(m_Mutex);
                m_StopRequested = false;
                m_ThreadID = thread::id();
                m_Cond.notify_all();
            }
            catch (...)
            {
            }
        }

    private:
        scoped_thread_id(scoped_thread_id const&);
        scoped_thread_id& operator= (scoped_thread_id const&);
    };

    //! A scope guard that resets a flag on destructor
    class scoped_flag
    {
    private:
        frontend_mutex_type& m_Mutex;
        condition_variable_any& m_Cond;
        volatile bool& m_Flag;

    public:
        explicit scoped_flag(frontend_mutex_type& mut, condition_variable_any& cond, volatile bool& f) :
            m_Mutex(mut), m_Cond(cond), m_Flag(f)
        {
        }
        ~scoped_flag()
        {
            try
            {
                lock_guard< frontend_mutex_type > lock(m_Mutex);
                m_Flag = false;
                m_Cond.notify_all();
            }
            catch (...)
            {
            }
        }

    private:
        scoped_flag(scoped_flag const&);
        scoped_flag& operator= (scoped_flag const&);
    };

public:
    //! Sink implementation type
    typedef SinkBackendT sink_backend_type;
    //! \cond
    BOOST_STATIC_ASSERT_MSG((has_requirement< typename sink_backend_type::frontend_requirements, synchronized_feeding >::value), "Asynchronous sink frontend is incompatible with the specified backend: thread synchronization requirements are not met");
    //! \endcond

#ifndef BOOST_LOG_DOXYGEN_PASS

    //! A pointer type that locks the backend until it's destroyed
    typedef boost::log::aux::locking_ptr< sink_backend_type > locked_backend_ptr;

#else // BOOST_LOG_DOXYGEN_PASS

    //! A pointer type that locks the backend until it's destroyed
    typedef implementation_defined locked_backend_ptr;

#endif // BOOST_LOG_DOXYGEN_PASS

private:
    //! Synchronization mutex
    backend_mutex_type m_BackendMutex;
    //! Pointer to the backend
    const shared_ptr< sink_backend_type > m_pBackend;

    //! Dedicated record feeding thread
    thread m_DedicatedFeedingThread;
    //! Feeding thread ID
    thread::id m_FeedingThreadID;
    //! Condition variable to implement blocking operations
    condition_variable_any m_BlockCond;

    //! The flag indicates that the feeding loop has to be stopped
    volatile bool m_StopRequested; // TODO: make it a real atomic
    //! The flag indicates that queue flush has been requested
    volatile bool m_FlushRequested; // TODO: make it a real atomic

public:
    /*!
     * Default constructor. Constructs the sink backend instance.
     * Requires the backend to be default-constructible.
     *
     * \param start_thread If \c true, the frontend creates a thread to feed
     *                     log records to the backend. Otherwise no thread is
     *                     started and it is assumed that the user will call
     *                     either \c run or \c feed_records himself.
     */
    asynchronous_sink(bool start_thread = true) :
        base_type(true),
        m_pBackend(boost::make_shared< sink_backend_type >()),
        m_StopRequested(false),
        m_FlushRequested(false)
    {
        if (start_thread)
            start_feeding_thread();
    }
    /*!
     * Constructor attaches user-constructed backend instance
     *
     * \param backend Pointer to the backend instance.
     * \param start_thread If \c true, the frontend creates a thread to feed
     *                     log records to the backend. Otherwise no thread is
     *                     started and it is assumed that the user will call
     *                     either \c run or \c feed_records himself.
     *
     * \pre \a backend is not \c NULL.
     */
    explicit asynchronous_sink(shared_ptr< sink_backend_type > const& backend, bool start_thread = true) :
        base_type(true),
        m_pBackend(backend),
        m_StopRequested(false),
        m_FlushRequested(false)
    {
        if (start_thread)
            start_feeding_thread();
    }

    // Constructors that pass arbitrary parameters to the backend constructor
    BOOST_LOG_PARAMETRIZED_CONSTRUCTORS_GEN(BOOST_LOG_SINK_CTOR_FORWARD_INTERNAL, ~)

    /*!
     * Destructor. Implicitly stops the dedicated feeding thread, if one is running.
     */
    ~asynchronous_sink()
    {
        boost::this_thread::disable_interruption no_interrupts;
        stop();
    }

    /*!
     * Locking accessor to the attached backend
     */
    locked_backend_ptr locked_backend()
    {
        return locked_backend_ptr(
            m_pBackend,
            static_cast< boost::log::aux::locking_ptr_counter_base& >(*this));
    }

    /*!
     * Enqueues the log record to the backend
     */
    void consume(record_view const& rec)
    {
        if (m_FlushRequested)
        {
            unique_lock< frontend_mutex_type > lock(base_type::frontend_mutex());
            // Wait until flush is done
            while (m_FlushRequested)
                m_BlockCond.wait(lock);
        }
        queue_base_type::enqueue(rec);
    }

    /*!
     * The method attempts to pass logging record to the backend
     */
    bool try_consume(record_view const& rec)
    {
        if (!m_FlushRequested)
        {
            return queue_base_type::try_enqueue(rec);
        }
        else
            return false;
    }

    /*!
     * The method starts record feeding loop and effectively blocks until either of this happens:
     *
     * \li the thread is interrupted due to either standard thread interruption or a call to \c stop
     * \li an exception is thrown while processing a log record in the backend, and the exception is
     *     not terminated by the exception handler, if one is installed
     *
     * \pre The sink frontend must be constructed without spawning a dedicated thread
     */
    void run()
    {
        // First check that no other thread is running
        scoped_thread_id guard(base_type::frontend_mutex(), m_BlockCond, m_FeedingThreadID, m_StopRequested);

        // Now start the feeding loop
        while (true)
        {
            do_feed_records();
            if (!m_StopRequested)
            {
                // Block until new record is available
                record_view rec;
                if (queue_base_type::dequeue_ready(rec))
                    base_type::feed_record(rec, m_BackendMutex, *m_pBackend);
            }
            else
                break;
        }
    }

    /*!
     * The method softly interrupts record feeding loop. This method must be called when the \c run
     * method execution has to be interrupted. Unlike regular thread interruption, calling
     * \c stop will not interrupt the record processing in the middle. Instead, the sink frontend
     * will attempt to finish its business with the record in progress and return afterwards.
     * This method can be called either if the sink was created with a dedicated thread,
     * or if the feeding loop was initiated by user.
     *
     * \note Returning from this method does not guarantee that there are no records left buffered
     *       in the sink frontend. It is possible that log records keep coming during and after this
     *       method is called. At some point of execution of this method log records stop being processed,
     *       and all records that come after this point are put into the queue. These records will be
     *       processed upon further calls to \c run or \c feed_records.
     */
    void stop()
    {
        unique_lock< frontend_mutex_type > lock(base_type::frontend_mutex());
        if (m_FeedingThreadID != thread::id() || m_DedicatedFeedingThread.joinable())
        {
            try
            {
                m_StopRequested = true;
                queue_base_type::interrupt_dequeue();
                while (m_StopRequested)
                    m_BlockCond.wait(lock);
            }
            catch (...)
            {
                m_StopRequested = false;
                throw;
            }

            lock.unlock();
            m_DedicatedFeedingThread.join();
        }
    }

    /*!
     * The method feeds log records that may have been buffered to the backend and returns
     *
     * \pre The sink frontend must be constructed without spawning a dedicated thread
     */
    void feed_records()
    {
        // First check that no other thread is running
        scoped_thread_id guard(base_type::frontend_mutex(), m_BlockCond, m_FeedingThreadID, m_StopRequested);

        // Now start the feeding loop
        do_feed_records();
    }

    /*!
     * The method feeds all log records that may have been buffered to the backend and returns.
     * Unlike \c feed_records, in case of ordering queueing the method also feeds records
     * that were enqueued during the ordering window, attempting to empty the queue completely.
     *
     * \pre The sink frontend must be constructed without spawning a dedicated thread
     */
    void flush()
    {
        unique_lock< frontend_mutex_type > lock(base_type::frontend_mutex());
        if (m_FeedingThreadID != thread::id() || m_DedicatedFeedingThread.joinable())
        {
            // There is already a thread feeding records, let it do the job
            m_FlushRequested = true;
            queue_base_type::interrupt_dequeue();
            while (!m_StopRequested && m_FlushRequested)
                m_BlockCond.wait(lock);

            // The condition may have been signalled when the feeding thread was finishing.
            // In that case records may not have been flushed, and we do the flush ourselves.
            if (m_FeedingThreadID != thread::id())
                return;
        }

        m_FlushRequested = true;

        // Flush records ourselves. The guard releases the lock.
        scoped_thread_id guard(lock, m_BlockCond, m_FeedingThreadID, m_StopRequested);

        do_feed_records();
    }

private:
#ifndef BOOST_LOG_DOXYGEN_PASS
    //! The method spawns record feeding thread
    void start_feeding_thread()
    {
        boost::thread(boost::bind(&asynchronous_sink::run, this)).swap(m_DedicatedFeedingThread);
    }

    // locking_ptr_counter_base methods
    void lock() { m_BackendMutex.lock(); }
    bool try_lock() { return m_BackendMutex.try_lock(); }
    void unlock() { m_BackendMutex.unlock(); }

    //! The record feeding loop
    void do_feed_records()
    {
        while (!m_StopRequested)
        {
            record_view rec;
            register bool dequeued = false;
            if (!m_FlushRequested)
                dequeued = queue_base_type::try_dequeue_ready(rec);
            else
                dequeued = queue_base_type::try_dequeue(rec);

            if (dequeued)
                base_type::feed_record(rec, m_BackendMutex, *m_pBackend);
            else
                break;
        }

        if (m_FlushRequested)
        {
            scoped_flag guard(base_type::frontend_mutex(), m_BlockCond, m_FlushRequested);
            base_type::flush_backend(m_BackendMutex, *m_pBackend);
        }
    }
#endif // BOOST_LOG_DOXYGEN_PASS
};

#undef BOOST_LOG_SINK_CTOR_FORWARD_INTERNAL

} // namespace sinks

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_SINKS_ASYNC_FRONTEND_HPP_INCLUDED_
