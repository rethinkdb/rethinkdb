// Copyright 2010 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>

namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::front;

#include <iostream>
#ifdef WIN32
#include "windows.h"
#else
#include <sys/time.h>
#endif

namespace test_fsm // Concrete FSM implementation
{
    // events
    struct play {};
    struct end_pause {};
    struct stop {};
    struct pause {};
    struct open_close {};
    struct cd_detected{};

    // Concrete FSM implementation 
    struct player_ : public msm::front::state_machine_def<player_>
    {
        // no need for exception handling or message queue
        typedef int no_exception_thrown;
        typedef int no_message_queue;

        // The list of FSM states
        struct Empty : public msm::front::state<> 
        {
            // optional entry/exit methods
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {/*std::cout << "entering: Empty" << std::endl;*/}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {/*std::cout << "leaving: Empty" << std::endl;*/}
        };
        struct Open : public msm::front::state<> 
        { 
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {/*std::cout << "entering: Open" << std::endl;*/}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {/*std::cout << "leaving: Open" << std::endl;*/}
        };

        struct Stopped : public msm::front::state<> 
        { 
            // when stopped, the CD is loaded
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {/*std::cout << "entering: Stopped" << std::endl;*/}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {/*std::cout << "leaving: Stopped" << std::endl;*/}
        };

        struct Playing : public msm::front::state<>
        {
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {/*std::cout << "entering: Playing" << std::endl;*/}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {/*std::cout << "leaving: Playing" << std::endl;*/}
        };

        // state not defining any entry or exit
        struct Paused : public msm::front::state<>
        {
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {/*std::cout << "entering: Paused" << std::endl;*/}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {/*std::cout << "leaving: Paused" << std::endl;*/}
        };

        // the initial state of the player SM. Must be defined
        typedef Empty initial_state;
        // transition actions
        struct start_playback
        {
            template <class FSM,class EVT,class SourceState,class TargetState>
            void operator()(EVT const& ,FSM&,SourceState& ,TargetState& )
            {
            }
        };
        struct open_drawer
        {
            template <class FSM,class EVT,class SourceState,class TargetState>
            void operator()(EVT const& ,FSM&,SourceState& ,TargetState& )
            {
            }
        };
        struct close_drawer
        {
            template <class FSM,class EVT,class SourceState,class TargetState>
            void operator()(EVT const& ,FSM&,SourceState& ,TargetState& )
            {
            }
        };
        struct store_cd_info
        {
            template <class FSM,class EVT,class SourceState,class TargetState>
            void operator()(EVT const&, FSM& fsm ,SourceState& ,TargetState& )
            {
            }
        };
        struct stop_playback 
        {
            template <class FSM,class EVT,class SourceState,class TargetState>
            void operator()(EVT const& ,FSM&,SourceState& ,TargetState& )
            {
            }
        };
        struct pause_playback
        {
            template <class FSM,class EVT,class SourceState,class TargetState>
            void operator()(EVT const& ,FSM&,SourceState& ,TargetState& )
            {
            }
        };
        struct resume_playback
        {
            template <class FSM,class EVT,class SourceState,class TargetState>
            void operator()(EVT const& ,FSM&,SourceState& ,TargetState& )
            {
            }
        };
        struct stop_and_open
        {
            template <class FSM,class EVT,class SourceState,class TargetState>
            void operator()(EVT const& ,FSM&,SourceState& ,TargetState& )
            {
            }
        };
        struct stopped_again
        {
            template <class FSM,class EVT,class SourceState,class TargetState>
            void operator()(EVT const& ,FSM&,SourceState& ,TargetState& )
            {
            }
        };
        // guard conditions

        // Transition table for player
        struct transition_table : mpl::vector<
            //    Start     Event         Next      Action                 Guard
            //    +---------+-------------+---------+---------------------+----------------------+
              Row < Stopped , play        , Playing , start_playback                            >,
              Row < Stopped , open_close  , Open    , open_drawer                               >,
              Row < Stopped , stop        , Stopped , stopped_again                             >,
            //    +---------+-------------+---------+---------------------+----------------------+
              Row < Open    , open_close  , Empty   , close_drawer                              >,
            //    +---------+-------------+---------+---------------------+----------------------+
              Row < Empty   , open_close  , Open    , open_drawer                               >,
              Row < Empty   , cd_detected , Stopped , store_cd_info                             >,
            //    +---------+-------------+---------+---------------------+----------------------+
              Row < Playing , stop        , Stopped , stop_playback                             >,
              Row < Playing , pause       , Paused  , pause_playback                            >,
              Row < Playing , open_close  , Open    , stop_and_open                             >,
            //    +---------+-------------+---------+---------------------+----------------------+
              Row < Paused  , end_pause   , Playing , resume_playback                           >,
              Row < Paused  , stop        , Stopped , stop_playback                             >,
              Row < Paused  , open_close  , Open    , stop_and_open                             >
            //    +---------+-------------+---------+---------------------+----------------------+
        > {};

        // Replaces the default no-transition response.
        template <class FSM,class Event>
        void no_transition(Event const& e, FSM&,int state)
        {
            std::cout << "no transition from state " << state
                << " on event " << typeid(e).name() << std::endl;
        }
    };
    typedef msm::back::state_machine<player_> player;

    //
    // Testing utilities.
    //
    static char const* const state_names[] = { "Stopped", "Open", "Empty", "Playing", "Paused" };

    void pstate(player const& p)
    {
        std::cout << " -> " << state_names[p.current_state()[0]] << std::endl;
    }

}

#ifndef WIN32
long mtime(struct timeval& tv1,struct timeval& tv2)
{
    return (tv2.tv_sec-tv1.tv_sec) *1000000 + ((tv2.tv_usec-tv1.tv_usec));
}
#endif


int main()
{
    // for timing
#ifdef WIN32
    LARGE_INTEGER res;
    ::QueryPerformanceFrequency(&res);
    LARGE_INTEGER li,li2;
#else
    struct timeval tv1,tv2;
    gettimeofday(&tv1,NULL);
#endif
    test_fsm::player p2;
    p2.start();
    // for timing
#ifdef WIN32
    ::QueryPerformanceCounter(&li);
#else
    gettimeofday(&tv1,NULL);
#endif
    for (int i=0;i<100;++i)
    {
        p2.process_event(test_fsm::open_close());
        p2.process_event(test_fsm::open_close()); 
        p2.process_event(test_fsm::cd_detected());
        p2.process_event(test_fsm::play());      
        p2.process_event(test_fsm::pause()); 
        // go back to Playing
        p2.process_event(test_fsm::end_pause()); 
        p2.process_event(test_fsm::pause()); 
        p2.process_event(test_fsm::stop());  
        // event leading to the same state
        p2.process_event(test_fsm::stop());
        p2.process_event(test_fsm::open_close());
        p2.process_event(test_fsm::open_close());
    }
#ifdef WIN32
    ::QueryPerformanceCounter(&li2);
#else
    gettimeofday(&tv2,NULL);
#endif
#ifdef WIN32
    std::cout << "msm took in s:" << (double)(li2.QuadPart-li.QuadPart)/res.QuadPart <<"\n" <<std::endl;
#else
    std::cout << "msm took in us:" <<  mtime(tv1,tv2) <<"\n" <<std::endl;
#endif
    return 0;
}

