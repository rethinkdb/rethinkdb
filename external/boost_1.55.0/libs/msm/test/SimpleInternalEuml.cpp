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
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/euml/euml.hpp>

#define BOOST_TEST_MODULE MyTest
#include <boost/test/unit_test.hpp>

using namespace std;
using namespace boost::msm::front::euml;
namespace msm = boost::msm;

namespace
{
    // A "complicated" event type that carries some data.
    enum DiskTypeEnum
    {
        DISK_CD=0,
        DISK_DVD=1
    };
    // events
    BOOST_MSM_EUML_EVENT(play)
    BOOST_MSM_EUML_EVENT(end_pause)
    BOOST_MSM_EUML_EVENT(stop)
    BOOST_MSM_EUML_EVENT(pause)
    BOOST_MSM_EUML_EVENT(open_close)
    BOOST_MSM_EUML_EVENT(internal_evt)
    BOOST_MSM_EUML_EVENT(to_ignore)
    // A "complicated" event type that carries some data.
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(std::string,cd_name)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(DiskTypeEnum,cd_type)
    BOOST_MSM_EUML_ATTRIBUTES((attributes_ << cd_name << cd_type ), cd_detected_attributes)
    BOOST_MSM_EUML_EVENT_WITH_ATTRIBUTES(cd_detected,cd_detected_attributes)

    //states
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(unsigned int,entry_counter)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(unsigned int,exit_counter)

    BOOST_MSM_EUML_STATE(( ++state_(entry_counter),++state_(exit_counter),attributes_ << entry_counter << exit_counter),Open)
    BOOST_MSM_EUML_STATE(( ++state_(entry_counter),++state_(exit_counter),attributes_ << entry_counter << exit_counter),Stopped)
    BOOST_MSM_EUML_STATE(( ++state_(entry_counter),++state_(exit_counter),attributes_ << entry_counter << exit_counter),Playing)
    BOOST_MSM_EUML_STATE(( ++state_(entry_counter),++state_(exit_counter),attributes_ << entry_counter << exit_counter),Paused)

    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(unsigned int,empty_internal_guard_counter)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(unsigned int,empty_internal_action_counter)

    BOOST_MSM_EUML_ACTION(internal_guard_fct)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        bool operator()(EVT const&, FSM& ,SourceState& src,TargetState& )
        {
            ++src.get_attribute(empty_internal_guard_counter);
            return false;
        }
    };
    BOOST_MSM_EUML_DECLARE_STATE((++state_(entry_counter),++state_(exit_counter),
                                  attributes_ << entry_counter << exit_counter 
                                              << empty_internal_guard_counter << empty_internal_action_counter),Empty_def)
    // derive to be able to add an internal transition table
    struct Empty_impl : public Empty_def
    {
        Empty_impl(){}
        BOOST_MSM_EUML_DECLARE_INTERNAL_TRANSITION_TABLE((
            internal_evt [internal_guard_fct] / ++source_(empty_internal_action_counter)
            ))
    };
    // declare an instance for the stt as we are manually declaring a state
    Empty_impl const Empty;

    //fsm
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(unsigned int,start_playback_counter)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(unsigned int,can_close_drawer_counter)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(unsigned int,internal_action_counter)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(unsigned int,internal_guard_counter)
    BOOST_MSM_EUML_ACTION(No_Transition)
    {
        template <class FSM,class Event>
        void operator()(Event const&,FSM&,int)
        {
            BOOST_FAIL("no_transition called!");
        }
    };
    BOOST_MSM_EUML_ACTION(good_disk_format)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        bool operator()(EVT const& evt,FSM&,SourceState& ,TargetState& )
        {
            if (evt.get_attribute(cd_type)!=DISK_CD)
            {
                return false;
            }
            return true;
        }
    };
    BOOST_MSM_EUML_ACTION(internal_guard)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        bool operator()(EVT const& evt,FSM& fsm,SourceState& ,TargetState& )
        {
            ++fsm.get_attribute(internal_guard_counter);
            return false;
        }
    };
    BOOST_MSM_EUML_ACTION(internal_guard2)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        bool operator()(EVT const& evt,FSM& fsm,SourceState& ,TargetState& )
        {
            ++fsm.get_attribute(internal_guard_counter);
            return true;
        }
    };
    BOOST_MSM_EUML_TRANSITION_TABLE((
          Playing   == Stopped  + play        / ++fsm_(start_playback_counter)                      ,
          Playing   == Paused   + end_pause   ,
          //  +------------------------------------------------------------------------------+
          Empty     == Open     + open_close  / ++fsm_(can_close_drawer_counter),
          Empty                 + to_ignore                                                         ,
          Empty                 + internal_evt [internal_guard2] / ++fsm_(internal_action_counter)  ,
          Empty                 + cd_detected [internal_guard]                                      ,
          //  +------------------------------------------------------------------------------+
          Open      == Empty    + open_close  ,
          Open      == Paused   + open_close  ,
          Open      == Stopped  + open_close  ,
          Open      == Playing  + open_close  ,
          //  +------------------------------------------------------------------------------+
          Paused    == Playing  + pause       ,
          //  +------------------------------------------------------------------------------+
          Stopped   == Playing  + stop        ,
          Stopped   == Paused   + stop        ,
          Stopped   == Empty    + cd_detected [good_disk_format]                                    ,
          Stopped   == Stopped  + stop                            
          //  +------------------------------------------------------------------------------+
         ),transition_table)

    BOOST_MSM_EUML_DECLARE_STATE_MACHINE(( transition_table, //STT
                                        init_ << Empty, // Init State
                                        no_action, // Entry
                                        no_action, // Exit
                                        attributes_ << start_playback_counter << can_close_drawer_counter 
                                                    << internal_action_counter << internal_guard_counter, // Attributes
                                        configure_ << no_configure_, // configuration
                                        No_Transition // no_transition handler
                                        ),
                                      player_) //fsm name

    typedef msm::back::state_machine<player_> player;

//    static char const* const state_names[] = { "Stopped", "Paused", "Open", "Empty", "Playing" };


    BOOST_AUTO_TEST_CASE( my_test )
    {     
        player p;

        p.start(); 
        BOOST_CHECK_MESSAGE(p.get_state<Empty_impl&>().get_attribute(entry_counter) == 1,
                            "Empty entry not called correctly");

        // internal events
        p.process_event(to_ignore);
        p.process_event(internal_evt);
        BOOST_CHECK_MESSAGE(p.get_attribute(internal_action_counter) == 1,"Internal action not called correctly");
        BOOST_CHECK_MESSAGE(p.get_attribute(internal_guard_counter) == 1,"Internal guard not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Empty_impl&>().get_attribute(empty_internal_action_counter) == 0,
                            "Empty internal action not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Empty_impl&>().get_attribute(empty_internal_guard_counter) == 1,
                            "Empty internal guard not called correctly");

        p.process_event(open_close()); 
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 2,"Open should be active"); //Open
        BOOST_CHECK_MESSAGE(p.get_state<Empty_impl&>().get_attribute(exit_counter) == 1,
                            "Empty exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Empty_impl&>().get_attribute(entry_counter) == 1,
                            "Open entry not called correctly");

        p.process_event(open_close()); 
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 3,"Empty should be active"); //Empty
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Open)&>().get_attribute(exit_counter) == 1,
                            "Open exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Empty_impl&>().get_attribute(entry_counter) == 2,
                            "Empty entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_attribute(can_close_drawer_counter) == 1,"guard not called correctly");

        p.process_event(
            cd_detected("louie, louie",DISK_DVD));
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 3,"Empty should be active"); //Empty
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Open)&>().get_attribute(exit_counter) == 1,
                            "Open exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Empty_impl&>().get_attribute(entry_counter) == 2,
                            "Empty entry not called correctly");

        p.process_event(
            cd_detected("louie, louie",DISK_CD)); 
        p.process_event(play);
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 4,"Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<Empty_impl&>().get_attribute(exit_counter) == 2,
                            "Empty exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Stopped)&>().get_attribute(entry_counter) == 1,
                            "Stopped entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Stopped)&>().get_attribute(exit_counter) == 1,
                            "Stopped exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Playing)&>().get_attribute(entry_counter) == 1,
                            "Playing entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_attribute(start_playback_counter) == 1,"action not called correctly");

        p.process_event(pause());
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 1,"Paused should be active"); //Paused
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Playing)&>().get_attribute(exit_counter) == 1,
                            "Playing exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Paused)&>().get_attribute(entry_counter) == 1,
                            "Paused entry not called correctly");

        // go back to Playing
        p.process_event(end_pause());  
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 4,"Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Paused)&>().get_attribute(exit_counter) == 1,
                            "Paused exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Playing)&>().get_attribute(entry_counter) == 2,
                            "Playing entry not called correctly");

        p.process_event(pause()); 
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 1,"Paused should be active"); //Paused
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Playing)&>().get_attribute(exit_counter) == 2,
                            "Playing exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Paused)&>().get_attribute(entry_counter) == 2,
                            "Paused entry not called correctly");

        p.process_event(stop());  
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 0,"Stopped should be active"); //Stopped
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Paused)&>().get_attribute(exit_counter) == 2,
                            "Paused exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Stopped)&>().get_attribute(entry_counter) == 2,
                            "Stopped entry not called correctly");

        p.process_event(stop());  
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 0,"Stopped should be active"); //Stopped
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Stopped)&>().get_attribute(exit_counter) == 2,
                            "Stopped exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Stopped)&>().get_attribute(entry_counter) == 3,
                            "Stopped entry not called correctly");
    }
}

