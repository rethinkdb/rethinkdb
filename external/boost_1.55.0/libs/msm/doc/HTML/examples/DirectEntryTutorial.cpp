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
            // every (optional) entry/exit methods get the event passed.
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "entering: State1" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "leaving: State1" << std::endl;}
        };
        struct State2 : public msm::front::state<> 
        {
            // every (optional) entry/exit methods get the event passed.
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "entering: State2" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "leaving: State2" << std::endl;}
        };
        struct SubFsm2_ : public msm::front::state_machine_def<SubFsm2_>
        {
            typedef msm::back::state_machine<SubFsm2_> SubFsm2;

            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "entering: SubFsm2" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "leaving: SubFsm2" << std::endl;}

            struct SubState1 : public msm::front::state<>
            {
                template <class Event,class FSM>
                void on_entry(Event const&,FSM& ) {std::cout << "entering: SubFsm2::SubState1" << std::endl;}
                template <class Event,class FSM>
                void on_exit(Event const&,FSM& ) {std::cout << "leaving: SubFsm2::SubState1" << std::endl;}
            };
            struct SubState1b : public msm::front::state<>
            {
                template <class Event,class FSM>
                void on_entry(Event const&,FSM& ) {std::cout << "entering: SubFsm2::SubState1b" << std::endl;}
                template <class Event,class FSM>
                void on_exit(Event const&,FSM& ) {std::cout << "leaving: SubFsm2::SubState1b" << std::endl;}
            };
            struct SubState2 : public msm::front::state<> , public msm::front::explicit_entry<0>
            {
                template <class Event,class FSM>
                void on_entry(Event const&,FSM& ) {std::cout << "entering: SubFsm2::SubState2" << std::endl;}
                template <class Event,class FSM>
                void on_exit(Event const&,FSM& ) {std::cout << "leaving: SubFsm2::SubState2" << std::endl;}
            };
            struct SubState2b : public msm::front::state<> , public msm::front::explicit_entry<1>
            {
                template <class Event,class FSM>
                void on_entry(Event const&,FSM& ) {std::cout << "entering: SubFsm2::SubState2b" << std::endl;}
                template <class Event,class FSM>
                void on_exit(Event const&,FSM& ) {std::cout << "leaving: SubFsm2::SubState2b" << std::endl;}
            };
            // test with a pseudo entry
            struct PseudoEntry1 : public msm::front::entry_pseudo_state<0>
            {
                template <class Event,class FSM>
                void on_entry(Event const&,FSM& ) {std::cout << "entering: SubFsm2::PseudoEntry1" << std::endl;}
                template <class Event,class FSM>
                void on_exit(Event const&,FSM& ) {std::cout << "leaving: SubFsm2::PseudoEntry1" << std::endl;}
            };
            struct SubState3 : public msm::front::state<>
            {
                template <class Event,class FSM>
                void on_entry(Event const&,FSM& ) {std::cout << "entering: SubFsm2::SubState3" << std::endl;}
                template <class Event,class FSM>
                void on_exit(Event const&,FSM& ) {std::cout << "leaving: SubFsm2::SubState3" << std::endl;}
            };
            struct SubState3b : public msm::front::state<>
            {
                template <class Event,class FSM>
                void on_entry(Event const&,FSM& ) {std::cout << "entering: SubFsm2::SubState3b" << std::endl;}
                template <class Event,class FSM>
                void on_exit(Event const&,FSM& ) {std::cout << "leaving: SubFsm2::SubState3b" << std::endl;}
            };
            struct PseudoExit1 : public msm::front::exit_pseudo_state<event6> 
            {
                template <class Event,class FSM>
                void on_entry(Event const&,FSM& ) {std::cout << "entering: SubFsm2::PseudoExit1" << std::endl;}
                template <class Event,class FSM>
                void on_exit(Event const&,FSM& ) {std::cout << "leaving: SubFsm2::PseudoExit1" << std::endl;}
            };
            // action methods
            void entry_action(event4 const&)
            {
                std::cout << "calling entry_action" << std::endl;
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
                std::cout << "no transition from state " << state
                    << " on event " << typeid(e).name() << std::endl;
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
            std::cout << "no transition from state " << state
                << " on event " << typeid(e).name() << std::endl;
        }
    };
    typedef msm::back::state_machine<Fsm_> Fsm;

    //
    // Testing utilities.
    //
    static char const* const state_names[] = { "State1", "SubFsm2","State2" };
    void pstate(Fsm const& p)
    {
        std::cout << " -> " << state_names[p.current_state()[0]] << std::endl;
    }

    void test()
    {
        Fsm p;
        // needed to start the highest-level SM. This will call on_entry and mark the start of the SM
        p.start(); 
        std::cout << "Simply move in and out of the composite, activate init states" << std::endl;
        p.process_event(event1()); pstate(p);
        p.process_event(event1()); pstate(p);
        std::cout << "Direct entry into SubFsm2::SubState2, then transition to SubState1 and back to State1" << std::endl;
        p.process_event(event2()); pstate(p);
        p.process_event(event6()); pstate(p);
        p.process_event(event1()); pstate(p);
        std::cout << "processing fork to SubFsm2::SubState2 and SubFsm2::SubState2b" << std::endl;
        p.process_event(event3()); pstate(p);
        p.process_event(event1()); pstate(p);
        std::cout << "processing entry pseudo state" << std::endl;
        p.process_event(event4()); pstate(p);
        p.process_event(event1()); pstate(p);
        std::cout << "processing entry + exit pseudo state" << std::endl;
        p.process_event(event4()); pstate(p);
        std::cout << "using exit pseudo state" << std::endl;
        p.process_event(event5()); pstate(p);
    }
}

int main()
{
    test();
    return 0;
}
