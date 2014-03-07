// Copyright 2010 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
// back-end
#include <boost/msm/back/state_machine.hpp>
//front-end
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>

namespace msm = boost::msm;
using namespace msm::front;

namespace
{
    // events
    struct play {};
    struct end_pause {};
    struct stop {};
    struct pause {};
    struct open_close {};

    // A "complicated" event type that carries some data.
    enum DiskTypeEnum
    {
        DISK_CD=0,
        DISK_DVD=1
    };
    struct cd_detected
    {
        cd_detected(std::string name, DiskTypeEnum diskType)
            : name(name),
            disc_type(diskType)
        {}

        std::string name;
        DiskTypeEnum disc_type;
    };

    // front-end: define the FSM structure 
    struct player_ : public msm::front::state_machine_def<player_>
    {
        // actions for Empty's internal transitions
        void internal_action(cd_detected const&){ std::cout << "Empty::internal action\n"; }
        bool internal_guard(cd_detected const&)
        {
            std::cout << "Empty::internal guard\n";
            return false;
        }
        struct internal_guard_fct 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            bool operator()(EVT const& evt ,FSM&,SourceState& ,TargetState& )
            {
                std::cout << "Empty::internal guard functor\n";
                return false;
            }
        };
        struct internal_action_fct 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            void operator()(EVT const& ,FSM& ,SourceState& ,TargetState& )
            {
                std::cout << "Empty::internal action functor" << std::endl;
            }
        };

        // The list of FSM states
        struct Empty : public msm::front::state<> 
        {
            // every (optional) entry/exit methods get the event passed.
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "entering: Empty" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "leaving: Empty" << std::endl;}

            // Transition table for Empty
            struct internal_transition_table : mpl::vector<
                //    Start     Event         Next      Action               Guard
           Internal <           cd_detected           , internal_action_fct ,internal_guard_fct    >
                //  +---------+-------------+---------+---------------------+----------------------+
            > {};        
        };
        struct Open : public msm::front::state<> 
        { 
            template <class Event,class FSM>
            void on_entry(Event const& ,FSM&) {std::cout << "entering: Open" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "leaving: Open" << std::endl;}
        };

        // sm_ptr still supported but deprecated as functors are a much better way to do the same thing
        struct Stopped : public msm::front::state<msm::front::default_base_state,msm::front::sm_ptr> 
        { 
            template <class Event,class FSM>
            void on_entry(Event const& ,FSM&) {std::cout << "entering: Stopped" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "leaving: Stopped" << std::endl;}
            void set_sm_ptr(player_* pl)
            {
                m_player=pl;
            }
            player_* m_player;
        };

        struct Playing : public msm::front::state<>
        {
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "entering: Playing" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "leaving: Playing" << std::endl;}
        };

        // state not defining any entry or exit
        struct Paused : public msm::front::state<>
        {
        };

        // the initial state of the player SM. Must be defined
        typedef Empty initial_state;

        // transition actions
        void start_playback(play const&)       { std::cout << "player::start_playback\n"; }
        void open_drawer(open_close const&)    { std::cout << "player::open_drawer\n"; }
        void close_drawer(open_close const&)   { std::cout << "player::close_drawer\n"; }
        void store_cd_info(cd_detected const&) { std::cout << "player::store_cd_info\n"; }
        void stop_playback(stop const&)        { std::cout << "player::stop_playback\n"; }
        void pause_playback(pause const&)      { std::cout << "player::pause_playback\n"; }
        void resume_playback(end_pause const&)      { std::cout << "player::resume_playback\n"; }
        void stop_and_open(open_close const&)  { std::cout << "player::stop_and_open\n"; }
        void stopped_again(stop const&)        {std::cout << "player::stopped_again\n";}
        // guard conditions
        bool good_disk_format(cd_detected const& evt)
        {
            // to test a guard condition, let's say we understand only CDs, not DVD
            if (evt.disc_type != DISK_CD)
            {
                std::cout << "wrong disk, sorry" << std::endl;
                return false;
            }
            return true;
        }

        typedef player_ p; // makes transition table cleaner

        // Transition table for player
        struct transition_table : mpl::vector<
            //    Start     Event         Next      Action               Guard
            //  +---------+-------------+---------+---------------------+----------------------+
          a_row < Stopped , play        , Playing , &p::start_playback                         >,
          a_row < Stopped , open_close  , Open    , &p::open_drawer                            >,
           _row < Stopped , stop        , Stopped                                              >,
            //  +---------+-------------+---------+---------------------+----------------------+
          a_row < Open    , open_close  , Empty   , &p::close_drawer                           >,
            //  +---------+-------------+---------+---------------------+----------------------+
          a_row < Empty   , open_close  , Open    , &p::open_drawer                            >,
          // conflict between a normal and 2 internal transitions (irow/g_irow)
            row < Empty   , cd_detected , Stopped , &p::store_cd_info   ,&p::good_disk_format  >,
           irow < Empty   , cd_detected ,           &p::internal_action ,&p::internal_guard    >,
         g_irow < Empty   , cd_detected                                 ,&p::internal_guard    >,
            //  +---------+-------------+---------+---------------------+----------------------+
          a_row < Playing , stop        , Stopped , &p::stop_playback                          >,
          a_row < Playing , pause       , Paused  , &p::pause_playback                         >,
          a_row < Playing , open_close  , Open    , &p::stop_and_open                          >,
            //  +---------+-------------+---------+---------------------+----------------------+
          a_row < Paused  , end_pause   , Playing , &p::resume_playback                        >,
          a_row < Paused  , stop        , Stopped , &p::stop_playback                          >,
          a_row < Paused  , open_close  , Open    , &p::stop_and_open                          >
            //  +---------+-------------+---------+---------------------+----------------------+
        > {};
        // Replaces the default no-transition response.
        template <class FSM,class Event>
        void no_transition(Event const& e, FSM&,int state)
        {
            std::cout << "no transition from state " << state
                << " on event " << typeid(e).name() << std::endl;
        }
        typedef int no_message_queue;

    };
    // Pick a back-end
    typedef msm::back::state_machine<player_> player;

    //
    // Testing utilities.
    //
    static char const* const state_names[] = { "Stopped", "Open", "Empty", "Playing", "Paused" };
    void pstate(player const& p)
    {
        std::cout << " -> " << state_names[p.current_state()[0]] << std::endl;
    }

    void test()
    {        
        player p;
        // needed to start the highest-level SM. This will call on_entry and mark the start of the SM
        p.start(); 
        // go to Open, call on_exit on Empty, then action, then on_entry on Open
        p.process_event(open_close()); pstate(p);
        p.process_event(open_close()); pstate(p);
        // will be rejected, wrong disk type
        p.process_event(
            cd_detected("louie, louie",DISK_DVD)); pstate(p);
        p.process_event(
            cd_detected("louie, louie",DISK_CD)); pstate(p);
        p.process_event(play());

        // at this point, Play is active      
        p.process_event(pause()); pstate(p);
        // go back to Playing
        p.process_event(end_pause());  pstate(p);
        p.process_event(pause()); pstate(p);
        p.process_event(stop());  pstate(p);
        // event leading to the same state
        // no action method called as it is not present in the transition table
        p.process_event(stop());  pstate(p);
    }
}

int main()
{
    test();
    return 0;
}
