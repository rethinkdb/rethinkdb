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

namespace msm = boost::msm;
using namespace boost::msm::front::euml;

namespace
{
    // events
    BOOST_MSM_EUML_EVENT(event1)

    BOOST_MSM_EUML_ACTION(State1_Entry)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "entering: State1" << std::endl;
        }
    };
    BOOST_MSM_EUML_ACTION(State1_Exit)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "leaving: State1" << std::endl;
        }
    };
    BOOST_MSM_EUML_ACTION(State2_Entry)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "entering: State2" << std::endl;
        }
    };
    BOOST_MSM_EUML_ACTION(State2_Exit)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "leaving: State2" << std::endl;
        }
    };
    BOOST_MSM_EUML_ACTION(State3_Entry)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "entering: State3" << std::endl;
        }
    };
    BOOST_MSM_EUML_ACTION(State3_Exit)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "leaving: State3" << std::endl;
        }
    };
    BOOST_MSM_EUML_ACTION(State4_Entry)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "entering: State4" << std::endl;
        }
    };
    BOOST_MSM_EUML_ACTION(State4_Exit)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "leaving: State4" << std::endl;
        }
    };

    // The list of FSM states
    BOOST_MSM_EUML_STATE(( State1_Entry,State1_Exit ),State1)
    BOOST_MSM_EUML_STATE(( State2_Entry,State2_Exit ),State2)
    BOOST_MSM_EUML_STATE(( State3_Entry,State3_Exit ),State3)
    BOOST_MSM_EUML_STATE(( State4_Entry,State4_Exit ),State4)

    // transition actions
    BOOST_MSM_EUML_ACTION(State2ToState3)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM&,SourceState& ,TargetState& )
        {
            std::cout << "my_machine::State2ToState3" << std::endl;
        }
    };
    BOOST_MSM_EUML_ACTION(State3ToState4)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM&,SourceState& ,TargetState& )
        {
            std::cout << "my_machine::State3ToState4" << std::endl;
        }
    };
    // guard conditions
    BOOST_MSM_EUML_ACTION(always_true)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        bool operator()(EVT const& evt,FSM&,SourceState& ,TargetState& )
        {
            std::cout << "always_true" << std::endl;
            return true;
        }
    };
    BOOST_MSM_EUML_ACTION(always_false)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        bool operator()(EVT const& evt,FSM&,SourceState& ,TargetState& )
        {
            std::cout << "always_false" << std::endl;
            return false;
        }
    };
    // replaces the old transition table
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
                                        init_ << State1  // Init State
                                        ),
                                      my_machine_) //fsm name

    // Pick a back-end
    typedef msm::back::state_machine<my_machine_> my_machine;

    //
    // Testing utilities.
    //
    static char const* const state_names[] = { "State1", "State2", "State3", "State4" };
    void pstate(my_machine const& p)
    {
        std::cout << " -> " << state_names[p.current_state()[0]] << std::endl;
    }

    void test()
    {        
        my_machine p;

        // needed to start the highest-level SM. This will call on_entry and mark the start of the SM
        // in this case it will also immediately trigger all anonymous transitions
        p.start(); 
        // this event will bring us back to the initial state and thus, a new "loop" will be started
        p.process_event(event1);
    }
}

int main()
{
    test();
    return 0;
}
