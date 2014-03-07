// Copyright 2010 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <vector>
#include <iostream>
// back-end
#include <boost/msm/back/state_machine.hpp>
//front-end
#include <boost/msm/front/state_machine_def.hpp>
// functors
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/front/euml/common.hpp>
// for And_ operator
#include <boost/msm/front/euml/operator.hpp>
// for func_state and func_state_machine
#include <boost/msm/front/euml/state_grammar.hpp>

using namespace std;
namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::front;
// for And_ operator
using namespace msm::front::euml;

namespace  // Concrete FSM implementation
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


    // The list of FSM states
    // entry and exit functors for Empty
    struct Empty_Entry
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "entering: Empty" << std::endl;
        }
    };
    struct Empty_Exit
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "leaving: Empty" << std::endl;
        }
    };
    // definition of Empty
    struct Empty_tag {};
    typedef msm::front::euml::func_state<Empty_tag,Empty_Entry,Empty_Exit> Empty; 

    struct Open_Entry
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "entering: Open" << std::endl;
        }
    };
    struct Open_Exit
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "leaving: Open" << std::endl;
        }
    };
    struct Open_tag {};
    typedef msm::front::euml::func_state<Open_tag,Open_Entry,Open_Exit> Open; 

    // states without entry/exit actions (can be declared as functor state, just without functors ;-) )
    struct Stopped_tag {};
    typedef msm::front::euml::func_state<Stopped_tag> Stopped; 

    struct Playing_tag {};
    typedef msm::front::euml::func_state<Playing_tag> Playing; 

    // state not defining any entry or exit (declared as simple state. Equivalent)
    struct Paused_tag {};
    typedef msm::front::euml::func_state<Paused_tag> Paused; 

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

    // Transition table for player
    struct player_transition_table : mpl::vector<
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
    // fsm definition
    struct player_tag {};
    typedef msm::front::euml::func_state_machine<Playing_tag,
                                                // transition table
                                                player_transition_table,
                                                //Initial state
                                                Empty> player_; 
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
        // no need to call play() as the previous event does it in its action method
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
