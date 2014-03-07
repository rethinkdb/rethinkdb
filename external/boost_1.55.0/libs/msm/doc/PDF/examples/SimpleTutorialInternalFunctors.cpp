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
// for And_ operator
#include <boost/msm/front/euml/operator.hpp>

using namespace std;
namespace msm = boost::msm;
using namespace msm::front;
namespace mpl = boost::mpl;
// for And_ operator
using namespace msm::front::euml;

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
        // The list of FSM states
        struct Empty : public msm::front::state<> 
        {
            // every (optional) entry/exit methods get the event passed.
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "entering: Empty" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "leaving: Empty" << std::endl;}
            struct internal_guard_fct 
            {
                template <class EVT,class FSM,class SourceState,class TargetState>
                bool operator()(EVT const& evt ,FSM&,SourceState& ,TargetState& )
                {
                    std::cout << "Empty::internal_transition_table guard\n";
                    return false;
                }
            };
            struct internal_action_fct 
            {
                template <class EVT,class FSM,class SourceState,class TargetState>
                void operator()(EVT const& ,FSM& ,SourceState& ,TargetState& )
                {
                    std::cout << "Empty::internal_transition_table action" << std::endl;
                }
            };
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

        struct Stopped : public msm::front::state<> 
        { 
            template <class Event,class FSM>
            void on_entry(Event const& ,FSM&) {std::cout << "entering: Stopped" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "leaving: Stopped" << std::endl;}
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
        // as the functors are generic on events, fsm and source/target state, 
        // you can reuse them in another machine if you wish
        struct TestFct 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            void operator()(EVT const&, FSM&,SourceState& ,TargetState& )
            {
                cout << "transition with event:" << typeid(EVT).name() << endl;
            }
        };
        struct start_playback 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            void operator()(EVT const& ,FSM& ,SourceState& ,TargetState& )
            {
                cout << "player::start_playback" << endl;
            }
        };
        struct open_drawer 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            void operator()(EVT const& ,FSM& ,SourceState& ,TargetState& )
            {
                cout << "player::open_drawer" << endl;
            }
        };
        struct close_drawer 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            void operator()(EVT const& ,FSM& ,SourceState& ,TargetState& )
            {
                cout << "player::close_drawer" << endl;
            }
        };
        struct store_cd_info 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            void operator()(EVT const&,FSM& fsm ,SourceState& ,TargetState& )
            {
                cout << "player::store_cd_info" << endl;
                fsm.process_event(play());
            }
        };
        struct stop_playback 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            void operator()(EVT const& ,FSM& ,SourceState& ,TargetState& )
            {
                cout << "player::stop_playback" << endl;
            }
        };
        struct pause_playback 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            void operator()(EVT const& ,FSM& ,SourceState& ,TargetState& )
            {
                cout << "player::pause_playback" << endl;
            }
        };
        struct resume_playback 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            void operator()(EVT const& ,FSM& ,SourceState& ,TargetState& )
            {
                cout << "player::resume_playback" << endl;
            }
        };
        struct stop_and_open 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            void operator()(EVT const& ,FSM& ,SourceState& ,TargetState& )
            {
                cout << "player::stop_and_open" << endl;
            }
        };
        struct stopped_again 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            void operator()(EVT const& ,FSM& ,SourceState& ,TargetState& )
            {
                cout << "player::stopped_again" << endl;
            }
        };
        // guard conditions
        struct DummyGuard 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            bool operator()(EVT const& evt,FSM& fsm,SourceState& src,TargetState& tgt)
            {
                return true;
            }
        };
        struct good_disk_format 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            bool operator()(EVT const& evt ,FSM&,SourceState& ,TargetState& )
            {
                // to test a guard condition, let's say we understand only CDs, not DVD
                if (evt.disc_type != DISK_CD)
                {
                    std::cout << "wrong disk, sorry" << std::endl;
                    return false;
                }
                return true;
            }
        };
        struct always_true 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            bool operator()(EVT const& evt ,FSM&,SourceState& ,TargetState& )
            {             
                return true;
            }
        };
        struct internal_guard 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            bool operator()(EVT const& evt ,FSM&,SourceState& ,TargetState& )
            {            
                std::cout << "Empty::internal guard\n";
                return false;
            }
        };
        // Transition table for player
        struct transition_table : mpl::vector<
            //    Start     Event         Next      Action                     Guard
            //  +---------+-------------+---------+---------------------------+----------------------+
            Row < Stopped , play        , Playing , ActionSequence_
                                                     <mpl::vector<
                                                     TestFct,start_playback> >            
                                                                              , DummyGuard           >,
            Row < Stopped , open_close  , Open    , open_drawer               , none                 >,
            Row < Stopped , stop        , Stopped , none                      , none                 >,
            //  +---------+-------------+---------+---------------------------+----------------------+
            Row < Open    , open_close  , Empty   , close_drawer              , none                 >,
            //  +---------+-------------+---------+---------------------------+----------------------+
            Row < Empty   , open_close  , Open    , open_drawer               , none                 >,
            Row < Empty   , cd_detected , Stopped , store_cd_info             , And_<good_disk_format,
                                                                                     always_true>    >,
            // internal transition inside the stt: none as Target
            Row < Empty   , cd_detected , none    , none                      , internal_guard       >,
            //  +---------+-------------+---------+---------------------------+----------------------+
            Row < Playing , stop        , Stopped , stop_playback             , none                 >,
            Row < Playing , pause       , Paused  , pause_playback            , none                 >,
            Row < Playing , open_close  , Open    , stop_and_open             , none                 >,
            //  +---------+-------------+---------+---------------------------+----------------------+
            Row < Paused  , end_pause   , Playing , resume_playback           , none                 >,
            Row < Paused  , stop        , Stopped , stop_playback             , none                 >,
            Row < Paused  , open_close  , Open    , stop_and_open             , none                 >
            //  +---------+-------------+---------+---------------------------+----------------------+
        > {};
        // Replaces the default no-transition response.
        template <class FSM,class Event>
        void no_transition(Event const& e, FSM&,int state)
        {
            std::cout << "no transition from state " << state
                << " on event " << typeid(e).name() << std::endl;
        }
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
        // no need to call play() as the previous transition does it in its action method
        //p.process_event(play());

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
