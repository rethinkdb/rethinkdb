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

namespace
{
    // events
    struct event1 {};
    struct event2 {};
    struct event3 {};
    struct event4 {};
    struct event5 {};
    struct event6 
    {
        event6(){}
        template <class Event>
        event6(Event const&){}
    };
    // front-end: define the FSM structure 
    struct Fsm_ : public msm::front::state_machine_def<Fsm_>
    {
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
        struct SubFsm2_ : public msm::front::state_machine_def<SubFsm2_>
        {
            typedef msm::back::state_machine<SubFsm2_> SubFsm2;

            unsigned int entry_action_counter;

            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {++entry_counter;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {++exit_counter;}
            int entry_counter;
            int exit_counter;

            struct SubState1 : public msm::front::state<>
            {
                template <class Event,class FSM>
                void on_entry(Event const&,FSM& ) {++entry_counter;}
                template <class Event,class FSM>
                void on_exit(Event const&,FSM& ) {++exit_counter;}
                int entry_counter;
                int exit_counter;
            };
            struct SubState1b : public msm::front::state<>
            {
                template <class Event,class FSM>
                void on_entry(Event const&,FSM& ) {++entry_counter;}
                template <class Event,class FSM>
                void on_exit(Event const&,FSM& ) {++exit_counter;}
                int entry_counter;
                int exit_counter;
            };
            struct SubState2 : public msm::front::state<> , public msm::front::explicit_entry<0>
            {
                template <class Event,class FSM>
                void on_entry(Event const&,FSM& ) {++entry_counter;}
                template <class Event,class FSM>
                void on_exit(Event const&,FSM& ) {++exit_counter;}
                int entry_counter;
                int exit_counter;
            };
            struct SubState2b : public msm::front::state<> , public msm::front::explicit_entry<1>
            {
                template <class Event,class FSM>
                void on_entry(Event const&,FSM& ) {++entry_counter;}
                template <class Event,class FSM>
                void on_exit(Event const&,FSM& ) {++exit_counter;}
                int entry_counter;
                int exit_counter;
            };
            // test with a pseudo entry
            struct PseudoEntry1 : public msm::front::entry_pseudo_state<0>
            {
                template <class Event,class FSM>
                void on_entry(Event const&,FSM& ) {++entry_counter;}
                template <class Event,class FSM>
                void on_exit(Event const&,FSM& ) {++exit_counter;}
                int entry_counter;
                int exit_counter;
            };
            struct SubState3 : public msm::front::state<>
            {
                template <class Event,class FSM>
                void on_entry(Event const&,FSM& ) {++entry_counter;}
                template <class Event,class FSM>
                void on_exit(Event const&,FSM& ) {++exit_counter;}
                int entry_counter;
                int exit_counter;
            };
            struct PseudoExit1 : public msm::front::exit_pseudo_state<event6> 
            {
                template <class Event,class FSM>
                void on_entry(Event const&,FSM& ) {++entry_counter;}
                template <class Event,class FSM>
                void on_exit(Event const&,FSM& ) {++exit_counter;}
                int entry_counter;
                int exit_counter;
            };
            // action methods
            void entry_action(event4 const&)
            {
                ++entry_action_counter;
            }
            // the initial state. Must be defined
            typedef mpl::vector<SubState1,SubState1b> initial_state;

            typedef mpl::vector<SubState2b> explicit_creation;

            // Transition table for SubFsm2
            struct transition_table : mpl::vector<
                //      Start          Event         Next         Action                  Guard
                //    +--------------+-------------+------------+------------------------+----------------------+
                a_row < PseudoEntry1 , event4      , SubState3  ,&SubFsm2_::entry_action                        >,
                _row  < SubState2    , event6      , SubState1                                                  >,
                _row  < SubState3    , event5      , PseudoExit1                                                >
                //    +--------------+-------------+------------+------------------------+----------------------+
            > {};
            // Replaces the default no-transition response.
            template <class FSM,class Event>
            void no_transition(Event const& e, FSM&,int state)
            {
                BOOST_FAIL("no_transition called!");
            }
        };
        typedef msm::back::state_machine<SubFsm2_> SubFsm2;

        // the initial state of the player SM. Must be defined
        typedef State1 initial_state;

        // transition actions
        // guard conditions

        // Transition table for Fsm
        struct transition_table : mpl::vector<
            //    Start                 Event    Next                                 Action  Guard
            //   +---------------------+--------+------------------------------------+-------+--------+
            _row < State1              , event1 , SubFsm2                                             >,
            _row < State1              , event2 , SubFsm2::direct<SubFsm2_::SubState2>                >,
            _row < State1              , event3 , mpl::vector<SubFsm2::direct<SubFsm2_::SubState2>,
                                                              SubFsm2::direct<SubFsm2_::SubState2b> > >,
            _row < State1              , event4 , SubFsm2::entry_pt
                                                        <SubFsm2_::PseudoEntry1>                      >,
            //   +---------------------+--------+------------------------------------+-------+--------+
            _row < SubFsm2             , event1 , State1                                              >,
            _row < SubFsm2::exit_pt
                <SubFsm2_::PseudoExit1>, event6 , State2                                              >
            //   +---------------------+--------+------------------------------------+-------+--------+
        > {};

        // Replaces the default no-transition response.
        template <class FSM,class Event>
        void no_transition(Event const& e, FSM&,int state)
        {
            BOOST_FAIL("no_transition called!");
        }
        // init counters
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& fsm) 
        {
            fsm.template get_state<Fsm_::State1&>().entry_counter=0;
            fsm.template get_state<Fsm_::State1&>().exit_counter=0;
            fsm.template get_state<Fsm_::State2&>().entry_counter=0;
            fsm.template get_state<Fsm_::State2&>().exit_counter=0;
            fsm.template get_state<Fsm_::SubFsm2&>().entry_counter=0;
            fsm.template get_state<Fsm_::SubFsm2&>().exit_counter=0;
            fsm.template get_state<Fsm_::SubFsm2&>().entry_action_counter=0;
            fsm.template get_state<Fsm_::SubFsm2&>().template get_state<Fsm_::SubFsm2::SubState1&>().entry_counter=0;
            fsm.template get_state<Fsm_::SubFsm2&>().template get_state<Fsm_::SubFsm2::SubState1&>().exit_counter=0;
            fsm.template get_state<Fsm_::SubFsm2&>().template get_state<Fsm_::SubFsm2::SubState1b&>().entry_counter=0;
            fsm.template get_state<Fsm_::SubFsm2&>().template get_state<Fsm_::SubFsm2::SubState1b&>().exit_counter=0;
            fsm.template get_state<Fsm_::SubFsm2&>().template get_state<Fsm_::SubFsm2::SubState2&>().entry_counter=0;
            fsm.template get_state<Fsm_::SubFsm2&>().template get_state<Fsm_::SubFsm2::SubState2&>().exit_counter=0;
            fsm.template get_state<Fsm_::SubFsm2&>().template get_state<Fsm_::SubFsm2::SubState2b&>().entry_counter=0;
            fsm.template get_state<Fsm_::SubFsm2&>().template get_state<Fsm_::SubFsm2::SubState2b&>().exit_counter=0;
            fsm.template get_state<Fsm_::SubFsm2&>().template get_state<Fsm_::SubFsm2::SubState3&>().entry_counter=0;
            fsm.template get_state<Fsm_::SubFsm2&>().template get_state<Fsm_::SubFsm2::SubState3&>().exit_counter=0;
            fsm.template get_state<Fsm_::SubFsm2&>().template get_state<Fsm_::SubFsm2::PseudoEntry1&>().entry_counter=0;
            fsm.template get_state<Fsm_::SubFsm2&>().template get_state<Fsm_::SubFsm2::PseudoEntry1&>().exit_counter=0;
            fsm.template get_state<Fsm_::SubFsm2&>().template get_state<Fsm_::SubFsm2::exit_pt<SubFsm2_::PseudoExit1>&>().entry_counter=0;
            fsm.template get_state<Fsm_::SubFsm2&>().template get_state<Fsm_::SubFsm2::exit_pt<SubFsm2_::PseudoExit1>&>().exit_counter=0;

        }
    };
    typedef msm::back::state_machine<Fsm_> Fsm;
//    static char const* const state_names[] = { "State1", "SubFsm2","State2"  };


    BOOST_AUTO_TEST_CASE( my_test )
    {     
        Fsm p;

        p.start(); 
        BOOST_CHECK_MESSAGE(p.get_state<Fsm_::State1&>().entry_counter == 1,"State1 entry not called correctly");

        p.process_event(event1()); 
        p.process_event(event1()); 
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 0,"State1 should be active");
        BOOST_CHECK_MESSAGE(p.get_state<Fsm_::State1&>().exit_counter == 1,"State1 exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Fsm_::State1&>().entry_counter == 2,"State1 entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Fsm_::SubFsm2&>().exit_counter == 1,"SubFsm2 exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Fsm_::SubFsm2&>().entry_counter == 1,"SubFsm2 entry not called correctly");

        p.process_event(event2()); 
        p.process_event(event6()); 
        p.process_event(event1()); 
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 0,"State1 should be active");
        BOOST_CHECK_MESSAGE(p.get_state<Fsm_::State1&>().exit_counter == 2,"State1 exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Fsm_::State1&>().entry_counter == 3,"State1 entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Fsm_::SubFsm2&>().exit_counter == 2,"SubFsm2 exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Fsm_::SubFsm2&>().entry_counter == 2,"SubFsm2 entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Fsm_::SubFsm2&>().get_state<Fsm_::SubFsm2_::SubState2&>().entry_counter == 1,
                            "SubFsm2::SubState2 entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Fsm_::SubFsm2&>().get_state<Fsm_::SubFsm2_::SubState2&>().exit_counter == 1,
                            "SubFsm2::SubState2 exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Fsm_::SubFsm2&>().get_state<Fsm_::SubFsm2_::SubState1&>().entry_counter == 2,
                            "SubFsm2::SubState1 entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Fsm_::SubFsm2&>().get_state<Fsm_::SubFsm2_::SubState1&>().exit_counter == 2,
                            "SubFsm2::SubState1 exit not called correctly");

        p.process_event(event3()); 
        p.process_event(event1()); 
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 0,"State1 should be active");
        BOOST_CHECK_MESSAGE(p.get_state<Fsm_::State1&>().exit_counter == 3,"State1 exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Fsm_::State1&>().entry_counter == 4,"State1 entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Fsm_::SubFsm2&>().exit_counter == 3,"SubFsm2 exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Fsm_::SubFsm2&>().entry_counter == 3,"SubFsm2 entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Fsm_::SubFsm2&>().get_state<Fsm_::SubFsm2_::SubState2&>().entry_counter == 2,
                            "SubFsm2::SubState2 entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Fsm_::SubFsm2&>().get_state<Fsm_::SubFsm2_::SubState2&>().exit_counter == 2,
                            "SubFsm2::SubState2 exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Fsm_::SubFsm2&>().get_state<Fsm_::SubFsm2_::SubState2b&>().entry_counter == 1,
                            "SubFsm2::SubState2b entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Fsm_::SubFsm2&>().get_state<Fsm_::SubFsm2_::SubState2b&>().exit_counter == 1,
                            "SubFsm2::SubState2b exit not called correctly");

        p.process_event(event4()); 
        p.process_event(event5()); 
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 2,"State2 should be active");
        BOOST_CHECK_MESSAGE(p.get_state<Fsm_::State1&>().exit_counter == 4,"State1 exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Fsm_::State2&>().entry_counter == 1,"State2 entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Fsm_::SubFsm2&>().exit_counter == 4,"SubFsm2 exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Fsm_::SubFsm2&>().entry_counter == 4,"SubFsm2 entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Fsm_::SubFsm2&>().get_state<Fsm_::SubFsm2_::PseudoEntry1&>().entry_counter == 1,
                            "SubFsm2::PseudoEntry1 entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Fsm_::SubFsm2&>().get_state<Fsm_::SubFsm2_::PseudoEntry1&>().exit_counter == 1,
                            "SubFsm2::PseudoEntry1 exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Fsm_::SubFsm2&>().get_state<Fsm_::SubFsm2_::SubState3&>().entry_counter == 1,
                            "SubFsm2::SubState3 entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Fsm_::SubFsm2&>().get_state<Fsm_::SubFsm2_::SubState3&>().exit_counter == 1,
                            "SubFsm2::SubState3 exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Fsm_::SubFsm2&>().get_state<Fsm_::SubFsm2::exit_pt<Fsm_::SubFsm2_::PseudoExit1>&>().entry_counter == 1,
                            "SubFsm2::PseudoExit1 entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Fsm_::SubFsm2&>().get_state<Fsm_::SubFsm2::exit_pt<Fsm_::SubFsm2_::PseudoExit1>&>().exit_counter == 1,
                            "SubFsm2::PseudoExit1 exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Fsm_::SubFsm2&>().entry_action_counter == 1,"Action not called correctly");

    }
}

