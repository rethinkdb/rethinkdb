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
#include <boost/msm/front/euml/euml.hpp>

#define BOOST_TEST_MODULE MyTest
#include <boost/test/unit_test.hpp>

namespace msm = boost::msm;
using namespace boost::msm::front::euml;

namespace
{
    // events
    BOOST_MSM_EUML_EVENT(event1)

    // The list of FSM states
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(unsigned int,entry_counter)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(unsigned int,exit_counter)

    BOOST_MSM_EUML_STATE(( ++state_(entry_counter),++state_(exit_counter),attributes_ << entry_counter << exit_counter),State1)
    BOOST_MSM_EUML_STATE(( ++state_(entry_counter),++state_(exit_counter),attributes_ << entry_counter << exit_counter),State2)
    BOOST_MSM_EUML_STATE(( ++state_(entry_counter),++state_(exit_counter),attributes_ << entry_counter << exit_counter),State3)
    BOOST_MSM_EUML_STATE(( ++state_(entry_counter),++state_(exit_counter),attributes_ << entry_counter << exit_counter),State4)

    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(unsigned int,state2_to_state3_counter)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(unsigned int,state3_to_state4_counter)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(unsigned int,always_true_counter)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(unsigned int,always_false_counter)

    // transition actions
    BOOST_MSM_EUML_ACTION(State2ToState3)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            ++fsm.get_attribute(state2_to_state3_counter);
        }
    };
    BOOST_MSM_EUML_ACTION(State3ToState4)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            ++fsm.get_attribute(state3_to_state4_counter);
        }
    };
    // guard conditions
    BOOST_MSM_EUML_ACTION(always_true)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        bool operator()(EVT const& evt,FSM& fsm,SourceState& ,TargetState& )
        {
            ++fsm.get_attribute(always_true_counter);
            return true;
        }
    };
    BOOST_MSM_EUML_ACTION(always_false)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        bool operator()(EVT const& evt,FSM& fsm,SourceState& ,TargetState& )
        {
            ++fsm.get_attribute(always_false_counter);
            return false;
        }
    };

    BOOST_MSM_EUML_ACTION(No_Transition)
    {
        template <class FSM,class Event>
        void operator()(Event const&,FSM&,int)
        {
            BOOST_FAIL("no_transition called!");
        }
    };

    BOOST_MSM_EUML_TRANSITION_TABLE((
          State2      == State1  ,
          State3      == State2 / State2ToState3,
          State4      == State3 [always_true] / State3ToState4,
          State4      == State3 [always_false],
          State1      == State4 + event1     
          //  +------------------------------------------------------------------------------+
          ),transition_table)

    // create a state machine "on the fly"
    BOOST_MSM_EUML_DECLARE_STATE_MACHINE(( transition_table, //STT
                                        init_ << State1,  // Init State
                                        no_action, // Entry
                                        no_action, // Exit
                                        attributes_ << state2_to_state3_counter << state3_to_state4_counter 
                                                    << always_true_counter << always_false_counter, // Attributes
                                        configure_ << no_configure_, // configuration
                                        No_Transition // no_transition handler
                                        ),
                                      my_machine_) //fsm name

    // Pick a back-end
    typedef msm::back::state_machine<my_machine_> my_machine;

    //static char const* const state_names[] = { "State1", "State2", "State3", "State4" };

    BOOST_AUTO_TEST_CASE( my_test )
    {        
        my_machine p;

        // needed to start the highest-level SM. This will call on_entry and mark the start of the SM
        // in this case it will also immediately trigger all anonymous transitions
        p.start(); 
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 3,"State4 should be active"); //State4
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(State1)&>().get_attribute(exit_counter) == 1,
                            "State1 exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(State1)&>().get_attribute(entry_counter) == 1,
                            "State1 entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(State2)&>().get_attribute(exit_counter) == 1,
                            "State2 exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(State2)&>().get_attribute(entry_counter) == 1,
                            "State2 entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(State3)&>().get_attribute(exit_counter) == 1,
                            "State3 exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(State3)&>().get_attribute(entry_counter) == 1,
                            "State3 entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(State4)&>().get_attribute(entry_counter)== 1,
                            "State4 entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_attribute(always_true_counter) == 1,"guard not called correctly");
        BOOST_CHECK_MESSAGE(p.get_attribute(always_false_counter) == 1,"guard not called correctly");
        BOOST_CHECK_MESSAGE(p.get_attribute(state2_to_state3_counter) == 1,"action not called correctly");
        BOOST_CHECK_MESSAGE(p.get_attribute(state3_to_state4_counter) == 1,"action not called correctly");


        // this event will bring us back to the initial state and thus, a new "loop" will be started
        p.process_event(event1);
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 3,"State4 should be active"); //State4
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(State1)&>().get_attribute(exit_counter) == 2,
                            "State1 exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(State1)&>().get_attribute(entry_counter) == 2,
                            "State1 entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(State2)&>().get_attribute(exit_counter) == 2,
                            "State2 exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(State2)&>().get_attribute(entry_counter) == 2,
                            "State2 entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(State3)&>().get_attribute(exit_counter) == 2,
                            "State3 exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(State3)&>().get_attribute(entry_counter) == 2,
                            "State3 entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(State4)&>().get_attribute(entry_counter)== 2,
                            "State4 entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_attribute(always_true_counter) == 2,"guard not called correctly");
        BOOST_CHECK_MESSAGE(p.get_attribute(always_false_counter) == 2,"guard not called correctly");
        BOOST_CHECK_MESSAGE(p.get_attribute(state2_to_state3_counter) == 2,"action not called correctly");
        BOOST_CHECK_MESSAGE(p.get_attribute(state3_to_state4_counter) == 2,"action not called correctly");

    }
}

