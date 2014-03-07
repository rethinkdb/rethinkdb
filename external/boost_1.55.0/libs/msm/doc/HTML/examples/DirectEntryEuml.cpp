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

#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/euml/euml.hpp>

using namespace std;
using namespace boost::msm::front::euml;
namespace msm = boost::msm;
namespace mpl = boost::mpl;


namespace  // Concrete FSM implementation
{
    // events
    BOOST_MSM_EUML_EVENT(event1)
    BOOST_MSM_EUML_EVENT(event2)
    BOOST_MSM_EUML_EVENT(event3)
    BOOST_MSM_EUML_EVENT(event4)
    BOOST_MSM_EUML_EVENT(event5)
    // if we need something special, like a template constructor, we cannot use the helper macros
    struct event6_impl : euml_event<event6_impl> 
    {
        event6_impl(){}
        template <class Event>
        event6_impl(Event const&){}
    };
    event6_impl const event6;

    //Sub fsm state definition
    BOOST_MSM_EUML_ACTION(SubState1_Entry)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "entering: SubFsm2::SubState1" << std::endl;
        }
    };
    BOOST_MSM_EUML_ACTION(SubState1_Exit)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "leaving: SubFsm2::SubState1" << std::endl;
        }
    };
    BOOST_MSM_EUML_STATE(( SubState1_Entry,SubState1_Exit ),SubState1)

    BOOST_MSM_EUML_ACTION(SubState1b_Entry)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "entering: SubFsm2::SubState1b" << std::endl;
        }
    };
    BOOST_MSM_EUML_ACTION(SubState1b_Exit)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "leaving: SubFsm2::SubState1b" << std::endl;
        }
    };
    BOOST_MSM_EUML_STATE(( SubState1b_Entry,SubState1b_Exit ),SubState1b)

    BOOST_MSM_EUML_ACTION(SubState1c_Entry)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "entering: SubFsm2::SubState1c" << std::endl;
        }
    };
    BOOST_MSM_EUML_ACTION(SubState1c_Exit)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "leaving: SubFsm2::SubState1c" << std::endl;
        }
    };
    BOOST_MSM_EUML_STATE(( SubState1c_Entry,SubState1c_Exit ),SubState1c)

    BOOST_MSM_EUML_ACTION(SubState2_Entry)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "entering: SubFsm2::SubState2" << std::endl;
        }
    };
    BOOST_MSM_EUML_ACTION(SubState2_Exit)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "leaving: SubFsm2::SubState2" << std::endl;
        }
    };
    BOOST_MSM_EUML_EXPLICIT_ENTRY_STATE(0,( SubState2_Entry,SubState2_Exit ),SubState2)

    BOOST_MSM_EUML_ACTION(SubState2b_Entry)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "entering: SubFsm2::SubState2b" << std::endl;
        }
    };
    BOOST_MSM_EUML_ACTION(SubState2b_Exit)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "leaving: SubFsm2::SubState2b" << std::endl;
        }
    };
    BOOST_MSM_EUML_EXPLICIT_ENTRY_STATE(1,( SubState2b_Entry,SubState2b_Exit ),SubState2b)

    BOOST_MSM_EUML_ACTION(SubState2c_Entry)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "entering: SubFsm2::SubState2c" << std::endl;
        }
    };
    BOOST_MSM_EUML_ACTION(SubState2c_Exit)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "leaving: SubFsm2::SubState2c" << std::endl;
        }
    };
    BOOST_MSM_EUML_EXPLICIT_ENTRY_STATE(2,( SubState2c_Entry,SubState2c_Exit ),SubState2c)

    BOOST_MSM_EUML_ACTION(PseudoEntry1_Entry)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "entering: SubFsm2::PseudoEntry1" << std::endl;
        }
    };
    BOOST_MSM_EUML_ACTION(PseudoEntry1_Exit)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "leaving: SubFsm2::PseudoEntry1" << std::endl;
        }
    };
    BOOST_MSM_EUML_ENTRY_STATE(0,( PseudoEntry1_Entry,PseudoEntry1_Exit ),PseudoEntry1)

    BOOST_MSM_EUML_ACTION(SubState3_Entry)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "entering: SubFsm2::SubState3" << std::endl;
        }
    };
    BOOST_MSM_EUML_ACTION(SubState3_Exit)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "leaving: SubFsm2::SubState3" << std::endl;
        }
    };
    BOOST_MSM_EUML_STATE(( SubState3_Entry,SubState3_Exit ),SubState3)

    BOOST_MSM_EUML_ACTION(SubState3b_Entry)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "entering: SubFsm2::SubState3b" << std::endl;
        }
    };
    BOOST_MSM_EUML_ACTION(SubState3b_Exit)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "leaving: SubFsm2::SubState3b" << std::endl;
        }
    };
    BOOST_MSM_EUML_STATE(( SubState3b_Entry,SubState3b_Exit ),SubState3b)

    BOOST_MSM_EUML_ACTION(PseudoExit1_Entry)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "entering: SubFsm2::PseudoExit1" << std::endl;
        }
    };
    BOOST_MSM_EUML_ACTION(PseudoExit1_Exit)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "leaving: SubFsm2::PseudoExit1" << std::endl;
        }
    };
    BOOST_MSM_EUML_EXIT_STATE(( event6,PseudoExit1_Entry,PseudoExit1_Exit ),PseudoExit1)

    // actions
    BOOST_MSM_EUML_ACTION(entry_action)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(FSM& ,EVT const& ,SourceState& ,TargetState& )
        {
            cout << "calling entry_action" << endl;
        }
    };
    // SubFsm definition
    BOOST_MSM_EUML_ACTION(SubFsm2_Entry)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "entering: SubFsm2" << std::endl;
        }
    };
    BOOST_MSM_EUML_ACTION(SubFsm2_Exit)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "leaving: SubFsm2" << std::endl;
        }
    };
    BOOST_MSM_EUML_TRANSITION_TABLE((
        //  +------------------------------------------------------------------------------+
            SubState3   == PseudoEntry1  + event4 / entry_action        ,
            SubState1   == SubState2     + event6                       ,
            PseudoExit1 == SubState3     + event5
        //  +------------------------------------------------------------------------------+
        ), SubFsm2_transition_table)

    BOOST_MSM_EUML_DECLARE_STATE_MACHINE( (SubFsm2_transition_table, //STT
                                    init_ << SubState1 << SubState1b << SubState1c, // Init State
                                    SubFsm2_Entry, // Entry
                                    SubFsm2_Exit
                                    ),SubFsm2_def)
    // inherit to add some typedef
    struct SubFsm2_ : public SubFsm2_def 
    {
        // these 2 states are not found in the transition table because they are accessed only through
        // a fork, so we need to create them explicitly
        typedef mpl::vector<BOOST_MSM_EUML_STATE_NAME(SubState2b),
                            BOOST_MSM_EUML_STATE_NAME(SubState2c)> explicit_creation;
    };

    // back-end
    typedef msm::back::state_machine<SubFsm2_> SubFsm2_type;
    SubFsm2_type const SubFsm2;

    // Fsm state definitions
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
    BOOST_MSM_EUML_STATE(( State1_Entry,State1_Exit ),State1)

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
    BOOST_MSM_EUML_STATE(( State2_Entry,State2_Exit ),State2)

    // Fsm definition
    BOOST_MSM_EUML_TRANSITION_TABLE((
        //  +------------------------------------------------------------------------------+
             SubFsm2                            == State1                + event1      ,
             explicit_(SubFsm2,SubState2)       == State1                + event2,
            (explicit_(SubFsm2,SubState2),
             explicit_(SubFsm2,SubState2b),
             explicit_(SubFsm2,SubState2c))     == State1                + event3      ,
             entry_pt_(SubFsm2,PseudoEntry1)    == State1                + event4      ,
             State1                             == SubFsm2               + event1      ,
             State2                             == exit_pt_
                                                  (SubFsm2,PseudoExit1)  + event6 
        //  +------------------------------------------------------------------------------+
        ),transition_table )


    BOOST_MSM_EUML_ACTION(Log_No_Transition)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const& e,FSM&,STATE& )
        {
            std::cout << "no transition in Fsm"
                << " on event " << typeid(e).name() << std::endl;
        }
    };
    BOOST_MSM_EUML_DECLARE_STATE_MACHINE(( transition_table, //STT
                                        init_ << State1, // Init State
                                        no_action, // Entry
                                        no_action, // Exit
                                        attributes_ << no_attributes_, // Attributes
                                        configure_ << no_configure_, // configuration
                                        Log_No_Transition // no_transition handler
                                        ),
                                      Fsm_) //fsm name

    //back-end
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
        p.process_event(event1); pstate(p);
        p.process_event(event1); pstate(p);
        std::cout << "Direct entry into SubFsm2::SubState2, then transition to SubState1 and back to State1" << std::endl;
        p.process_event(event2); pstate(p);
        p.process_event(event6); pstate(p);
        p.process_event(event1); pstate(p);
        std::cout << "processing fork to SubFsm2::SubState2, SubFsm2::SubState2b and SubFsm2::SubState2c" << std::endl;
        p.process_event(event3); pstate(p);
        p.process_event(event1); pstate(p);
        std::cout << "processing entry pseudo state" << std::endl;
        p.process_event(event4); pstate(p);
        p.process_event(event1); pstate(p);
        std::cout << "processing entry + exit pseudo state" << std::endl;
        p.process_event(event4); pstate(p);
        std::cout << "using exit pseudo state" << std::endl;
        p.process_event(event5); pstate(p);
    }
}

int main()
{
    test();
     return 0;
}
