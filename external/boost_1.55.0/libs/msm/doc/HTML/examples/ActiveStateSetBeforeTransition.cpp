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
// functors
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/front/euml/common.hpp>


using namespace std;
namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::front;

namespace  // Concrete FSM implementation
{
    // events
    struct disconnect {};
    struct connect {};

    // flag
    struct is_connected{};
    // front-end: define the FSM structure 
    struct Connection_ : public msm::front::state_machine_def<Connection_>
    {
        // when a transition is about to be taken, we already update our currently active state(s)
        typedef msm::active_state_switch_before_transition active_state_switch_policy;

        // The list of FSM states
        struct Connected : public msm::front::state<> 
        {
            // in this state, we are connected
            typedef mpl::vector1<is_connected>      flag_list;
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "entering: Connected" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "leaving: Connected" << std::endl;}
        };
        struct Disconnected : public msm::front::state<> 
        { 
            template <class Event,class FSM>
            void on_entry(Event const& ,FSM&) {std::cout << "entering: Disconnected" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "leaving: Disconnected" << std::endl;}
        };

        // transition actions
        struct SignalConnect 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            void operator()(EVT const&, FSM& fsm,SourceState& ,TargetState& )
            {
                // by default, this would be wrong (shows false)
                cout << "SignalConnect. Connected? " << std::boolalpha << fsm.template is_flag_active<is_connected>() << endl;
            }
        };
        struct SignalDisconnect 
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            void operator()(EVT const&, FSM& fsm,SourceState& ,TargetState& )
            {
                // by default, this would be wrong (shows true)
                cout << "SignalDisconnect. Connected? " << std::boolalpha << fsm.template is_flag_active<is_connected>() << endl;
            }
        };
 
        // the initial state of the player SM. Must be defined
        typedef Disconnected initial_state;

        // Transition table for player
        struct transition_table : mpl::vector<
            //    Start          Event         Next           Action                     Guard
            //  +--------------+-------------+--------------+---------------------------+----------------------+
            Row < Connected    , disconnect  , Disconnected , SignalDisconnect          , none                 >,
            Row < Disconnected , connect     , Connected    , SignalConnect             , none                 >
            //  +--------------+-------------+--------------+---------------------------+----------------------+
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
    typedef msm::back::state_machine<Connection_> Connection;


    void test()
    {        
        Connection connection;
        // needed to start the highest-level SM. This will call on_entry and mark the start of the SM
        connection.start(); 
        // signal a connection
        connection.process_event(connect());
        // signal a disconnection
        connection.process_event(disconnect());
        connection.stop();
    }
}

int main()
{
    test();
    return 0;
}
