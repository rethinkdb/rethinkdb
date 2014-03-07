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

#include <boost/phoenix/phoenix.hpp>

// add phoenix support in eUML
#define BOOST_MSM_EUML_PHOENIX_SUPPORT
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/euml/euml.hpp>


using namespace std;
using namespace boost::msm::front::euml;
namespace msm = boost::msm;
using namespace boost::phoenix;

// entry/exit/action/guard logging functors
#include "logging_functors.h"

namespace  // Concrete FSM implementation
{
    // events
    BOOST_MSM_EUML_EVENT(end_pause)
    BOOST_MSM_EUML_EVENT(stop)
    BOOST_MSM_EUML_EVENT(pause)
    BOOST_MSM_EUML_EVENT(open_close)
    struct play_event : boost::msm::front::euml::euml_event<play_event>
    {
    };
    play_event play;

    enum DiskTypeEnum
    {
        DISK_CD=0,
        DISK_DVD=1
    };
    // A "complicated" event type that carries some data.
    struct cd_detected_event : boost::msm::front::euml::euml_event<cd_detected_event>
    {
        cd_detected_event(){}
        cd_detected_event(std::string const& name,DiskTypeEnum disk):cd_name(name),cd_type(disk){}
        std::string cd_name;
        DiskTypeEnum cd_type;
    };
    // define an instance for a nicer transition table
    cd_detected_event cd_detected;

    // Concrete FSM implementation 
    // The list of FSM states
    // state not needing any entry or exit
    BOOST_MSM_EUML_STATE((),Paused)

    // states with standard eUML actions
    BOOST_MSM_EUML_STATE(( Stopped_Entry,Stopped_Exit ),Stopped)
    BOOST_MSM_EUML_STATE(( Playing_Entry,Playing_Exit ),Playing)

    // a "standard" msm state
    struct Empty_impl : public msm::front::state<> , public euml_state<Empty_impl>
    {
        // this allows us to add some functions
        void foo() {std::cout << "Empty::foo " << std::endl;}
        // standard entry behavior
        template <class Event,class FSM>
        void on_entry(Event const& evt,FSM& fsm) 
        {
            std::cout << "entering: Empty" << std::endl;
        }
        template <class Event,class FSM>
        void on_exit(Event const& evt,FSM& fsm) 
        {
            std::cout << "leaving: Empty" << std::endl;
        }
    };
    //instance for use in the transition table
    Empty_impl const Empty;

    // entry and exit actions as phoenix functions
    struct open_entry_impl
    {
        typedef void result_type;
        void operator()()
        {
            cout << "entering: Open" << endl;
        }
    };
    boost::phoenix::function<open_entry_impl> open_entry;
    struct open_exit_impl
    {
        typedef void result_type;
        void operator()()
        {
            cout << "leaving: Open" << endl;
        }
    };
    boost::phoenix::function<open_exit_impl> open_exit;

    // a state using phoenix for entry/exit actions
    BOOST_MSM_EUML_STATE(( open_entry(),open_exit() ),Open)

    // actions and guards using boost::phoenix
    struct start_playback_impl
    {
        typedef void result_type;
        void operator()()
        {
            cout << "calling: start_playback" << endl;
        }
    };
    boost::phoenix::function<start_playback_impl> start_playback;

    // a guard taking the event as argument
    struct good_disk_format_impl
    {
        typedef bool result_type;

        template <class Event>
        bool operator()(Event const& evt)
        {
            // to test a guard condition, let's say we understand only CDs, not DVD
            if (evt.cd_type!=DISK_CD)
            {
                std::cout << "wrong disk, sorry" << std::endl;
                return false;
            }
            std::cout << "good disk" << std::endl;
            return true;
        }
    };
    boost::phoenix::function<good_disk_format_impl> good_disk_format;

    // a simple action
    struct store_cd_info_impl
    {
        typedef void result_type;
        void operator()()
        {
            cout << "calling: store_cd_info" << endl;
        }
    };
    boost::phoenix::function<store_cd_info_impl> store_cd_info;

    // an action taking the fsm as argument and sending it a new event
    struct process_play_impl
    {
        typedef void result_type;

        template <class Fsm>
        void operator()(Fsm& fsm)
        {
            cout << "queuing a play event" << endl;
            fsm.process_event(play);
        }
    };
    // it is also possible to use BOOST_PHOENIX_ADAPT_CALLABLE to avoid defining a global variable
    BOOST_PHOENIX_ADAPT_CALLABLE(process_play, process_play_impl, 1)

 
    // transition table. Actions and guards are written as phoenix functions
    BOOST_MSM_EUML_TRANSITION_TABLE((
          //an action without arguments
          Playing   == Stopped  + play        / start_playback()                            , 
          Playing   == Paused   + end_pause                                                 ,
          //  +------------------------------------------------------------------------------+
          Empty     == Open     + open_close                                                ,
          //  +------------------------------------------------------------------------------+
          Open      == Empty    + open_close                                                ,
          Open      == Paused   + open_close                                                ,
          Open      == Stopped  + open_close                                                ,
          Open      == Playing  + open_close                                                ,
          //  +------------------------------------------------------------------------------+
          Paused    == Playing  + pause                                                     ,
          //  +------------------------------------------------------------------------------+
          Stopped   == Playing  + stop                                                      ,
          Stopped   == Paused   + stop                                                      ,
          // a guard taking the event as argument
          // and an action made of a phoenix expression of 2 actions
          // _event is a placeholder for the current event
          // _fsm is a placeholder for the current state machine
          Stopped   == Empty    + cd_detected [good_disk_format(_event)] 
                                              / (store_cd_info(),process_play(_fsm)),
          Stopped   == Stopped  + stop                            
          //  +------------------------------------------------------------------------------+
         ),transition_table)

    // create a state machine "on the fly"
    BOOST_MSM_EUML_DECLARE_STATE_MACHINE(( transition_table, //STT
                                        init_ << Empty, // Init State
                                        no_action, // Entry
                                        no_action, // Exit
                                        attributes_ << no_attributes_, // Attributes
                                        configure_ << no_configure_, // configuration
                                        Log_No_Transition // no_transition handler
                                        ),
                                      player_) //fsm name

    // or simply, if no no_transition handler needed:
    //BOOST_MSM_EUML_DECLARE_STATE_MACHINE(( transition_table, //STT
    //                                    Empty // Init State
    //                                 ),player_)

    // choice of back-end
    typedef msm::back::state_machine<player_> player;

    //
    // Testing utilities.
    //
    static char const* const state_names[] = { "Stopped", "Paused", "Open", "Empty", "Playing" };
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
        p.process_event(open_close); pstate(p);
        p.process_event(open_close); pstate(p);
        // will be rejected, wrong disk type
        p.process_event(
            cd_detected_event("louie, louie",DISK_DVD)); pstate(p);
        p.process_event(
            cd_detected_event("louie, louie",DISK_CD)); pstate(p);
        // no need to call play as the previous event does it in its action method
        //p.process_event(play);

        // at this point, Play is active      
        p.process_event(pause); pstate(p);
        // go back to Playing
        p.process_event(end_pause);  pstate(p);
        p.process_event(pause); pstate(p);
        p.process_event(stop);  pstate(p);
        // event leading to the same state
        // no action method called as none is defined in the transition table
        p.process_event(stop);  pstate(p);
        // test call to no_transition
        p.process_event(pause); pstate(p);
    }
}

int main()
{
    test();
    return 0;
}

