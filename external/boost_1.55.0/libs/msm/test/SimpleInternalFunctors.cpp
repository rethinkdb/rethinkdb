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
#include <boost/msm/front/euml/common.hpp>

#define BOOST_TEST_MODULE MyTest
#include <boost/test/unit_test.hpp>

using namespace std;
namespace msm = boost::msm;
using namespace msm::front;
namespace mpl = boost::mpl;

namespace
{
    // events
    struct play {};
    struct end_pause {};
    struct stop {};
    struct pause {};
    struct open_close {};
    struct internal_evt {};
    struct to_ignore {};

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
        unsigned int start_playback_counter;
        unsigned int can_close_drawer_counter;
        unsigned int internal_action_counter;
        unsigned int internal_guard_counter;

        player_():
        start_playback_counter(0),
        can_close_drawer_counter(0),
        internal_action_counter(0),
        internal_guard_counter(0)
        {}

        // The list of FSM states
        struct Empty : public msm::front::state<> 
        {
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {++entry_counter;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {++exit_counter;}
            int entry_counter;
            int exit_counter;
            unsigned int empty_internal_guard_counter;
            unsigned int empty_internal_action_counter;
            struct internal_guard_fct 
            {
                template <class EVT,class FSM,class SourceState,class TargetState>
                bool operator()(EVT const& evt ,FSM&,SourceState& src,TargetState& )
                {
                    ++src.empty_internal_guard_counter;
                    return false;
                }
            };
            struct internal_action_fct 
            {
                template <class EVT,class FSM,class SourceState,class TargetState>
                void operator()(EVT const& ,FSM& ,SourceState& src,TargetState& )
                {
                    ++src.empty_internal_action_counter;
                }
            };
            // Transition table for Empty
            struct internal_transition_table : mpl::vector<
                //    Start     Event         Next      Action               Guard
           Internal <           internal_evt          , internal_action_fct ,internal_guard_fct    >
                //  +---------+-------------+---------+---------------------+----------------------+
            > {};        
        };
        struct Open : public msm::front::state<> 
        { 
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {++entry_counter;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {++exit_counter;}
            int entry_counter;
            int exit_counter;
        };

        // sm_ptr still supported but deprecated as functors are a much better way to do the same thing
        struct Stopped : public msm::front::state<> 
        { 
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {++entry_counter;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {++exit_counter;}
            int entry_counter;
            int exit_counter;
        };

        struct Playing : public msm::front::state<>
        {
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {++entry_counter;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {++exit_counter;}
            int entry_counter;
            int exit_counter;
        };

        // state not defining any entry or exit
        struct Paused : public msm::front::state<>
        {
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {++entry_counter;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {++exit_counter;}
            int entry_counter;
            int exit_counter;
        };

        // the initial state of the player SM. Must be defined
        typedef Empty initial_state;

        // transition actions
        void start_playback(play const&)       {++start_playback_counter; }
        void open_drawer(open_close const&)    {  }
        void store_cd_info(cd_detected const&) {  }
        void stop_playback(stop const&)        {  }
        void pause_playback(pause const&)      {  }
        void resume_playback(end_pause const&)      {  }
        void stop_and_open(open_close const&)  {  }
        void stopped_again(stop const&){}
        struct internal_action 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            void operator()(EVT const& evt ,FSM& fsm,SourceState& ,TargetState& )
            {            
                ++fsm.internal_action_counter;
            }
        };
        struct internal_guard 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            bool operator()(EVT const& evt ,FSM& fsm,SourceState& ,TargetState& )
            {            
                ++fsm.internal_guard_counter;
                return false;
            }
        };
        struct internal_guard2 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            bool operator()(EVT const& evt ,FSM& fsm,SourceState& ,TargetState& )
            {            
                ++fsm.internal_guard_counter;
                return true;
            }
        };
        // guard conditions
        bool good_disk_format(cd_detected const& evt)
        {
            // to test a guard condition, let's say we understand only CDs, not DVD
            if (evt.disc_type != DISK_CD)
            {
                return false;
            }
            return true;
        }
        bool can_close_drawer(open_close const&)   
        {
            ++can_close_drawer_counter;
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
          g_row < Open    , open_close  , Empty   ,                      &p::can_close_drawer  >,
            //  +---------+-------------+---------+---------------------+----------------------+
          a_row < Empty   , open_close  , Open    , &p::open_drawer                            >,
            row < Empty   , cd_detected , Stopped , &p::store_cd_info   ,&p::good_disk_format  >,
            Row < Empty   , internal_evt, none    , internal_action     ,internal_guard2       >,
            Row < Empty   , to_ignore   , none    , none                , none                 >,
            Row < Empty   , cd_detected , none    , none                ,internal_guard        >,
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
        void no_transition(Event const&, FSM&,int)
        {
            BOOST_FAIL("no_transition called!");
        }
        // init counters
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& fsm) 
        {
            fsm.template get_state<player_::Stopped&>().entry_counter=0;
            fsm.template get_state<player_::Stopped&>().exit_counter=0;
            fsm.template get_state<player_::Open&>().entry_counter=0;
            fsm.template get_state<player_::Open&>().exit_counter=0;
            fsm.template get_state<player_::Empty&>().entry_counter=0;
            fsm.template get_state<player_::Empty&>().exit_counter=0;
            fsm.template get_state<player_::Empty&>().empty_internal_guard_counter=0;
            fsm.template get_state<player_::Empty&>().empty_internal_action_counter=0;
            fsm.template get_state<player_::Playing&>().entry_counter=0;
            fsm.template get_state<player_::Playing&>().exit_counter=0;
            fsm.template get_state<player_::Paused&>().entry_counter=0;
            fsm.template get_state<player_::Paused&>().exit_counter=0;
        }

    };
    // Pick a back-end
    typedef msm::back::state_machine<player_> player;

//    static char const* const state_names[] = { "Stopped", "Open", "Empty", "Playing", "Paused" };


    BOOST_AUTO_TEST_CASE( my_test )
    {     
        player p;

        p.start(); 
        BOOST_CHECK_MESSAGE(p.get_state<player_::Empty&>().entry_counter == 1,"Empty entry not called correctly");
        // internal events
        p.process_event(to_ignore());
        p.process_event(internal_evt());
        BOOST_CHECK_MESSAGE(p.internal_action_counter == 1,"Internal action not called correctly");
        BOOST_CHECK_MESSAGE(p.internal_guard_counter == 1,"Internal guard not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Empty&>().empty_internal_action_counter == 0,"Empty internal action not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Empty&>().empty_internal_guard_counter == 1,"Empty internal guard not called correctly");

        p.process_event(open_close()); 
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 1,"Open should be active"); //Open
        BOOST_CHECK_MESSAGE(p.get_state<player_::Empty&>().exit_counter == 1,"Empty exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Open&>().entry_counter == 1,"Open entry not called correctly");

        p.process_event(open_close()); 
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 2,"Empty should be active"); //Empty
        BOOST_CHECK_MESSAGE(p.get_state<player_::Open&>().exit_counter == 1,"Open exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Empty&>().entry_counter == 2,"Empty entry not called correctly");
        BOOST_CHECK_MESSAGE(p.can_close_drawer_counter == 1,"guard not called correctly");

        p.process_event(
            cd_detected("louie, louie",DISK_DVD));
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 2,"Empty should be active"); //Empty
        BOOST_CHECK_MESSAGE(p.get_state<player_::Open&>().exit_counter == 1,"Open exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Empty&>().entry_counter == 2,"Empty entry not called correctly");
        BOOST_CHECK_MESSAGE(p.internal_guard_counter == 2,"Internal guard not called correctly");

        p.process_event(
            cd_detected("louie, louie",DISK_CD)); 
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 0,"Stopped should be active"); //Stopped
        BOOST_CHECK_MESSAGE(p.get_state<player_::Empty&>().exit_counter == 2,"Empty exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Stopped&>().entry_counter == 1,"Stopped entry not called correctly");
        BOOST_CHECK_MESSAGE(p.internal_guard_counter == 3,"Internal guard not called correctly");

        p.process_event(play());
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 3,"Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<player_::Stopped&>().exit_counter == 1,"Stopped exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Playing&>().entry_counter == 1,"Playing entry not called correctly");
        BOOST_CHECK_MESSAGE(p.start_playback_counter == 1,"action not called correctly");

        p.process_event(pause());
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 4,"Paused should be active"); //Paused
        BOOST_CHECK_MESSAGE(p.get_state<player_::Playing&>().exit_counter == 1,"Playing exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Paused&>().entry_counter == 1,"Paused entry not called correctly");

        // go back to Playing
        p.process_event(end_pause());  
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 3,"Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<player_::Paused&>().exit_counter == 1,"Paused exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Playing&>().entry_counter == 2,"Playing entry not called correctly");

        p.process_event(pause()); 
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 4,"Paused should be active"); //Paused
        BOOST_CHECK_MESSAGE(p.get_state<player_::Playing&>().exit_counter == 2,"Playing exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Paused&>().entry_counter == 2,"Paused entry not called correctly");

        p.process_event(stop());  
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 0,"Stopped should be active"); //Stopped
        BOOST_CHECK_MESSAGE(p.get_state<player_::Paused&>().exit_counter == 2,"Paused exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Stopped&>().entry_counter == 2,"Stopped entry not called correctly");

        p.process_event(stop());  
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 0,"Stopped should be active"); //Stopped
        BOOST_CHECK_MESSAGE(p.get_state<player_::Stopped&>().exit_counter == 2,"Stopped exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Stopped&>().entry_counter == 3,"Stopped entry not called correctly");
    }
}

