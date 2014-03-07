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

#define BOOST_TEST_MODULE MyTest
#include <boost/test/unit_test.hpp>

namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace boost::msm::front;

namespace
{
    // events
    struct event1 {};


    // front-end: define the FSM structure 
    struct my_machine_ : public msm::front::state_machine_def<my_machine_>
    {
        unsigned int state2_to_state3_counter;
        unsigned int state3_to_state4_counter;
        unsigned int always_true_counter;
        unsigned int always_false_counter;

        my_machine_():
        state2_to_state3_counter(0),
        state3_to_state4_counter(0),
        always_true_counter(0),
        always_false_counter(0)
        {}

        // The list of FSM states
        struct State1 : public msm::front::state<> 
        {
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {++entry_counter;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {++exit_counter;}
            int entry_counter;
            int exit_counter;
        };
        struct State2 : public msm::front::state<> 
        {
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {++entry_counter;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {++exit_counter;}
            int entry_counter;
            int exit_counter;
        };

        struct State3 : public msm::front::state<> 
        {
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {++entry_counter;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {++exit_counter;}
            int entry_counter;
            int exit_counter;
        };

        struct State4 : public msm::front::state<>
        {
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {++entry_counter;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {++exit_counter;}
            int entry_counter;
            int exit_counter;
        };

        // the initial state of the player SM. Must be defined
        typedef State1 initial_state;

        // transition actions
        void State2ToState3(none const&)       { ++state2_to_state3_counter; }
        void State3ToState4(none const&)       { ++state3_to_state4_counter; }
        // guard conditions
        bool always_true(none const& evt)
        {
            ++always_true_counter;
            return true;
        }
        bool always_false(none const& evt)
        {
            ++always_false_counter;
            return false;
        }

        typedef my_machine_ p; // makes transition table cleaner

        // Transition table for player
        struct transition_table : mpl::vector<
            //    Start     Event         Next      Action               Guard
            //  +---------+-------------+---------+---------------------+----------------------+
           _row < State1  , none        , State2                                               >,
          a_row < State2  , none        , State3  , &p::State2ToState3                         >,
            //  +---------+-------------+---------+---------------------+----------------------+
            row < State3  , none        , State4  , &p::State3ToState4  , &p::always_true      >,
          g_row < State3  , none        , State4                        , &p::always_false     >,
           _row < State4  , event1      , State1                                               >
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
            fsm.template get_state<my_machine_::State1&>().entry_counter=0;
            fsm.template get_state<my_machine_::State1&>().exit_counter=0;
            fsm.template get_state<my_machine_::State2&>().entry_counter=0;
            fsm.template get_state<my_machine_::State2&>().exit_counter=0;
            fsm.template get_state<my_machine_::State3&>().entry_counter=0;
            fsm.template get_state<my_machine_::State3&>().exit_counter=0;
            fsm.template get_state<my_machine_::State4&>().entry_counter=0;
            fsm.template get_state<my_machine_::State4&>().exit_counter=0;
        }
    };
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
        BOOST_CHECK_MESSAGE(p.get_state<my_machine_::State1&>().exit_counter == 1,"State1 exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<my_machine_::State1&>().entry_counter == 1,"State1 entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<my_machine_::State2&>().exit_counter == 1,"State2 exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<my_machine_::State2&>().entry_counter == 1,"State2 entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<my_machine_::State3&>().exit_counter == 1,"State3 exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<my_machine_::State3&>().entry_counter == 1,"State3 entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<my_machine_::State4&>().entry_counter == 1,"State4 entry not called correctly");
        BOOST_CHECK_MESSAGE(p.always_true_counter == 1,"guard not called correctly");
        BOOST_CHECK_MESSAGE(p.always_false_counter == 1,"guard not called correctly");
        BOOST_CHECK_MESSAGE(p.state2_to_state3_counter == 1,"action not called correctly");
        BOOST_CHECK_MESSAGE(p.state3_to_state4_counter == 1,"action not called correctly");


        // this event will bring us back to the initial state and thus, a new "loop" will be started
        p.process_event(event1());
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 3,"State4 should be active"); //State4
        BOOST_CHECK_MESSAGE(p.get_state<my_machine_::State1&>().exit_counter == 2,"State1 exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<my_machine_::State1&>().entry_counter == 2,"State1 entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<my_machine_::State2&>().exit_counter == 2,"State2 exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<my_machine_::State2&>().entry_counter == 2,"State2 entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<my_machine_::State3&>().exit_counter == 2,"State3 exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<my_machine_::State3&>().entry_counter == 2,"State3 entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<my_machine_::State4&>().entry_counter == 2,"State4 entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<my_machine_::State4&>().exit_counter == 1,"State4 exit not called correctly");
        BOOST_CHECK_MESSAGE(p.always_true_counter == 2,"guard not called correctly");
        BOOST_CHECK_MESSAGE(p.always_false_counter == 2,"guard not called correctly");
        BOOST_CHECK_MESSAGE(p.state2_to_state3_counter == 2,"action not called correctly");
        BOOST_CHECK_MESSAGE(p.state3_to_state4_counter == 2,"action not called correctly");

    }
}

