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
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/euml/euml.hpp>

using namespace std;
namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace boost::msm::front::euml;


namespace // Concrete FSM implementation
{
    // events
    BOOST_MSM_EUML_EVENT(connect)
    BOOST_MSM_EUML_EVENT(disconnect)

    // flag
    BOOST_MSM_EUML_FLAG(is_connected)

    BOOST_MSM_EUML_ACTION(SignalConnect)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            // by default, this would be wrong (shows false)
            cout << "SignalConnect. Connected? " 
                 << std::boolalpha 
                 << fsm.template is_flag_active<BOOST_MSM_EUML_FLAG_NAME(is_connected)>() << endl;
        }
    };
    BOOST_MSM_EUML_ACTION(SignalDisconnect)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            // by default, this would be wrong (shows true)
            cout << "SignalDisconnect. Connected? " 
                 << std::boolalpha 
                 << fsm.template is_flag_active<BOOST_MSM_EUML_FLAG_NAME(is_connected)>() 
                 << endl;
        }
    };

    // The list of FSM states
    BOOST_MSM_EUML_ACTION(Connected_Entry)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "entering: Connected" << std::endl;
        }
    };
    BOOST_MSM_EUML_ACTION(Connected_Exit)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "leaving: Connected" << std::endl;
        }
    };
    BOOST_MSM_EUML_STATE(( Connected_Entry,Connected_Exit,
                           attributes_ << no_attributes_,
                           configure_<< is_connected ),Connected)

    BOOST_MSM_EUML_ACTION(Disconnected_Entry)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "entering: Disconnected" << std::endl;
        }
    };
    BOOST_MSM_EUML_ACTION(Disconnected_Exit)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "leaving: Disconnected" << std::endl;
        }
    };
    BOOST_MSM_EUML_STATE(( Disconnected_Entry,Disconnected_Exit ),Disconnected)

    // replaces the old transition table
    BOOST_MSM_EUML_TRANSITION_TABLE((
          Disconnected  == Connected    + disconnect    / SignalDisconnect  ,
          Connected     == Disconnected + connect       / SignalConnect 
          //  +------------------------------------------------------------------------------+
          ),transition_table)

    BOOST_MSM_EUML_ACTION(Log_No_Transition)
    {
        template <class FSM,class Event>
        void operator()(Event const& e,FSM&,int state)
        {
            std::cout << "no transition from state " << state
                << " on event " << typeid(e).name() << std::endl;
        }
    };

    // create a state machine "on the fly"
    BOOST_MSM_EUML_DECLARE_STATE_MACHINE(( transition_table, //STT
                                        init_ << Disconnected, // Init State
                                        no_action, // Entry
                                        no_action, // Exit
                                        attributes_ << no_attributes_, // Attributes
                                        configure_ << switch_active_before_transition, // configuration
                                        Log_No_Transition // no_transition handler
                                        ),
                                      Connection_) //fsm name

    typedef msm::back::state_machine<Connection_> Connection;

    void test()
    {        
        Connection connection;
        // needed to start the highest-level SM. This will call on_entry and mark the start of the SM
        connection.start(); 
        // signal a connection
        connection.process_event(connect);
        // signal a disconnection
        connection.process_event(disconnect);
        connection.stop();
    }
}

int main()
{
    test();
    return 0;
}


