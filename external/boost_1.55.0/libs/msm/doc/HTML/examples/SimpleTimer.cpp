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

// how long the timer will ring when countdown elapsed.
#define RINGING_TIME 5

namespace  // Concrete FSM implementation
{
    // events
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(int,m_timer)
    BOOST_MSM_EUML_ATTRIBUTES((attributes_ << m_timer ), start_timer_attr)
    BOOST_MSM_EUML_EVENT_WITH_ATTRIBUTES(start_timer,start_timer_attr)

    BOOST_MSM_EUML_EVENT(stop_timer)

    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(int,m_tick)
    BOOST_MSM_EUML_ATTRIBUTES((attributes_ << m_tick ), tick_attr)
    BOOST_MSM_EUML_EVENT_WITH_ATTRIBUTES(tick,tick_attr)

    BOOST_MSM_EUML_EVENT(start_ringing)

    // Concrete FSM implementation 

    // The list of FSM states
    BOOST_MSM_EUML_ACTION(Stopped_Entry)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "entering: Stopped" << std::endl;
        }
    };
    BOOST_MSM_EUML_STATE(( Stopped_Entry ),Stopped)

    BOOST_MSM_EUML_ACTION(Started_Entry)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "entering: Started" << std::endl;
        }
    };

    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(int,m_counter)
    BOOST_MSM_EUML_STATE((  Started_Entry,
                            no_action,
                            attributes_ << m_counter
                  ),
                  Started)

    BOOST_MSM_EUML_ACTION(Ringing_Entry)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "entering: Ringing" << std::endl;
        }
    };
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(int,m_ringing_cpt)
    BOOST_MSM_EUML_STATE((  Ringing_Entry,
                            no_action,
                            attributes_ << m_ringing_cpt
                  ),
                  Ringing)

    // external function
    void do_ring(int ringing_time) {std::cout << "ringing " << ringing_time << " s" << std::endl;}
    // create functor and eUML function
    BOOST_MSM_EUML_FUNCTION(Ring_ , do_ring , ring_ , void , void )

    // replaces the old transition table
    BOOST_MSM_EUML_TRANSITION_TABLE((
        //  +------------------------------------------------------------------------------+
          // When we start the countdown, the countdown value is not hardcoded but contained in the start_timer event.
          // We copy this value into Started
          Started     == Stopped + start_timer /(target_(m_counter)= event_(m_timer))         ,
          Stopped     == Started + stop_timer                                                 ,
          // internal transition
          Started + tick
                    // we here use the message queue to move to Started when the countdown is finished
                    // to do this we put start_ringing into the message queue
                    / if_then_( (source_(m_counter) -= event_(m_tick) ) <= Int_<0>(),
                                 process_(start_ringing) )                                        ,
          // when we start ringing, we give to the state its hard-coded ringing time.
          Ringing     == Started + start_ringing 
                           / (target_(m_ringing_cpt) = Int_<RINGING_TIME>(),
                              // call the external do_ring function
                              ring_(Int_<RINGING_TIME>()))                                          ,
          // to change a bit, we now do not use the message queue but a transition conflict to solve the same problem.
          // When tick is fired, we have an internal transition Ringing -> Ringing, as long as Counter > 0
          Ringing + tick [ source_(m_ringing_cpt) - event_(m_tick) > Int_<0>() ] 
                             /(target_(m_ringing_cpt) -= event_(m_tick) )                           ,
          // And we move to Stopped when the counter is 0
          Stopped     == Ringing + tick[source_(m_ringing_cpt)-event_(m_tick) <= Int_<0>()]   ,
          // we let the user manually stop the ringing by pressing any button
          Stopped     == Ringing + stop_timer                                                 ,
          Stopped     == Ringing + start_timer
          //  +------------------------------------------------------------------------------+
          ),transition_table)

    // create a state machine "on the fly"
    BOOST_MSM_EUML_DECLARE_STATE_MACHINE(( transition_table, //STT
                                        init_ << Stopped // Init State
                                        ),
                                      SimpleTimer_) //fsm name

    // choice of back-end
    typedef msm::back::state_machine<SimpleTimer_> SimpleTimer;

    //
    // Testing utilities.
    //
    static char const* const state_names[] = { "Stopped", "Started","Ringing" };
    void pstate(SimpleTimer const& p)
    {
        std::cout << " -> " << state_names[p.current_state()[0]] << std::endl;
    }

    void test()
    {        
        SimpleTimer p;
        // needed to start the highest-level SM. This will call on_entry and mark the start of the SM
        p.start();

        p.process_event(start_timer(5));pstate(p); //timer set to 5 ticks
        p.process_event(tick(2));pstate(p);
        p.process_event(tick(1));pstate(p);
        p.process_event(tick(1));pstate(p);
        p.process_event(tick(1));pstate(p);
        // we are now ringing, let it ring a bit
        p.process_event(tick(2));pstate(p);
        p.process_event(tick(1));pstate(p);
        p.process_event(tick(1));pstate(p);
        p.process_event(tick(1));pstate(p);
    }
}

int main()
{
    test();
    return 0;
}
