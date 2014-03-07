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

namespace msm = boost::msm;
namespace mpl = boost::mpl;

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
    struct NextSong {};
    struct PreviousSong {};


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

        struct Playing_ : public msm::front::state_machine_def<Playing_>
        {
            // no need for exception handling or message queue
            typedef int no_exception_thrown;
            typedef int no_message_queue;

            // The list of FSM states
            struct Song1 : public msm::front::state<> {};
            struct Song2 : public msm::front::state<> {};
            struct Song3 : public msm::front::state<> {};
            // the initial state. Must be defined
            typedef Song1 initial_state;
            // transition actions
            void start_next_song(NextSong const&)       { /*std::cout << "Playing::start_next_song\n";*/ }
            void start_prev_song(PreviousSong const&)       { /*std::cout << "Playing::start_prev_song\n"; */}
            // guard conditions

            typedef Playing_ pl; // makes transition table cleaner
            // Transition table for Playing
            struct transition_table : mpl::vector4<
                //    Start     Event         Next      Action                 Guard
                //    +---------+-------------+---------+---------------------+----------------------+
                a_row < Song1   , NextSong    , Song2   , &pl::start_next_song                     >,
                a_row < Song2   , PreviousSong, Song1   , &pl::start_prev_song                     >,
                a_row < Song2   , NextSong    , Song3   , &pl::start_next_song                     >,
                a_row < Song3   , PreviousSong, Song2   , &pl::start_prev_song                     >
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
        typedef msm::back::state_machine<Playing_> Playing;

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
        void start_playback(play const&)       {  }
        void open_drawer(open_close const&)    {  }
        void close_drawer(open_close const&)   {  }
        void store_cd_info(cd_detected const& cd) { }
        void stop_playback(stop const&)        {  }
        void pause_playback(pause const&)      { }
        void resume_playback(end_pause const&)      {  }
        void stop_and_open(open_close const&)  {  }
        void stopped_again(stop const&){}
        // guard conditions

        typedef player_ p; // makes transition table cleaner

        // Transition table for player
        struct transition_table : mpl::vector<
            //    Start     Event         Next      Action                 Guard
            //    +---------+-------------+---------+---------------------+----------------------+
            a_row < Stopped , play        , Playing , &p::start_playback                       >,
            a_row < Stopped , open_close  , Open    , &p::open_drawer                          >,
            a_row < Stopped , stop        , Stopped , &p::stopped_again                        >,
            //    +---------+-------------+---------+---------------------+----------------------+
            a_row < Open    , open_close  , Empty   , &p::close_drawer                         >,
            //    +---------+-------------+---------+---------------------+----------------------+
            a_row < Empty   , open_close  , Open    , &p::open_drawer                          >,
            a_row < Empty   , cd_detected , Stopped , &p::store_cd_info                        >,
            //    +---------+-------------+---------+---------------------+----------------------+
            a_row < Playing , stop        , Stopped , &p::stop_playback                        >,
            a_row < Playing , pause       , Paused  , &p::pause_playback                       >,
            a_row < Playing , open_close  , Open    , &p::stop_and_open                        >,
            //    +---------+-------------+---------+---------------------+----------------------+
            a_row < Paused  , end_pause   , Playing , &p::resume_playback                      >,
            a_row < Paused  , stop        , Stopped , &p::stop_playback                        >,
            a_row < Paused  , open_close  , Open    , &p::stop_and_open                        >
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
        for (int j=0;j<100;++j)
        {
            p2.process_event(test_fsm::NextSong());
            p2.process_event(test_fsm::NextSong());
            p2.process_event(test_fsm::PreviousSong());
            p2.process_event(test_fsm::PreviousSong());
        }

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

