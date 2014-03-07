// Copyright 2010 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "stdafx.h"

#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/euml/euml.hpp>
#include <boost/msm/front/euml/stl.hpp>

namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace boost::msm::front::euml;

#include <iostream>
#ifdef WIN32
#include "windows.h"
#else
#include <sys/time.h>
#endif

namespace // Concrete FSM implementation
{
    // events
    BOOST_MSM_EUML_EVENT(play)
    BOOST_MSM_EUML_EVENT(end_pause)
    BOOST_MSM_EUML_EVENT(stop)
    BOOST_MSM_EUML_EVENT(pause)
    BOOST_MSM_EUML_EVENT(open_close)
    BOOST_MSM_EUML_EVENT(cd_detected)

    BOOST_MSM_EUML_ACTION(start_playback)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM&,SourceState& ,TargetState& )
        {
        }
    };
    BOOST_MSM_EUML_ACTION(open_drawer)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM&,SourceState& ,TargetState& )
        {
        }
    };
    BOOST_MSM_EUML_ACTION(close_drawer)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM&,SourceState& ,TargetState& )
        {
        }
    };
    BOOST_MSM_EUML_ACTION(store_cd_info)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const&, FSM& fsm ,SourceState& ,TargetState& )
        {
        }
    };
    BOOST_MSM_EUML_ACTION(stop_playback)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM&,SourceState& ,TargetState& )
        {
        }
    };
    BOOST_MSM_EUML_ACTION(pause_playback)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM&,SourceState& ,TargetState& )
        {
        }
    };
    BOOST_MSM_EUML_ACTION(resume_playback)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM&,SourceState& ,TargetState& )
        {
        }
    };
    BOOST_MSM_EUML_ACTION(stop_and_open)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM&,SourceState& ,TargetState& )
        {
        }
    };
    BOOST_MSM_EUML_ACTION(stopped_again)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM&,SourceState& ,TargetState& )
        {
        }
    };

    // The list of FSM states
    BOOST_MSM_EUML_STATE((),Empty)
    BOOST_MSM_EUML_STATE((),Open)
    BOOST_MSM_EUML_STATE((),Stopped)
    BOOST_MSM_EUML_STATE((),Playing)
    BOOST_MSM_EUML_STATE((),Paused)

    // replaces the old transition table
    BOOST_MSM_EUML_TRANSITION_TABLE((
          Playing   == Stopped  + play        / start_playback  ,
          Playing   == Paused   + end_pause   / resume_playback ,
          //  +------------------------------------------------------------------------------+
          Empty     == Open     + open_close  / close_drawer    ,
          //  +------------------------------------------------------------------------------+
          Open      == Empty    + open_close  / open_drawer     ,
          Open      == Paused   + open_close  / stop_and_open   ,
          Open      == Stopped  + open_close  / open_drawer     ,
          Open      == Playing  + open_close  / stop_and_open   ,
          //  +------------------------------------------------------------------------------+
          Paused    == Playing  + pause       / pause_playback  ,
          //  +------------------------------------------------------------------------------+
          Stopped   == Playing  + stop        / stop_playback   ,
          Stopped   == Paused   + stop        / stop_playback   ,
          Stopped   == Empty    + cd_detected / store_cd_info   ,
          Stopped   == Stopped  + stop        / stopped_again                    
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
                                        init_ << Empty, // Init State
                                        no_action, // Entry
                                        no_action, // Exit
                                        attributes_ << no_attributes_, // Attributes
                                        configure_ << no_exception << no_msg_queue, // configuration
                                        Log_No_Transition // no_transition handler
                                        ),
                                      player_) //fsm name

    typedef msm::back::state_machine<player_> player;

    //
    // Testing utilities.
    //
    static char const* const state_names[] = { "Stopped", "Open", "Empty", "Playing", "Paused" };

    void pstate(player const& p)
    {
        std::cout << " -> " << state_names[p.current_state()[0]] << std::endl;
    }

}

#ifndef WIN32
long mtime(struct timeval& tv1,struct timeval& tv2)
{
    return (tv2.tv_sec-tv1.tv_sec) *1000000 + ((tv2.tv_usec-tv1.tv_usec));
}
#endif


int main()
{
    // for timing
#ifdef WIN32
    LARGE_INTEGER res;
    ::QueryPerformanceFrequency(&res);
    LARGE_INTEGER li,li2;
#else
    struct timeval tv1,tv2;
    gettimeofday(&tv1,NULL);
#endif

    player p2;
    p2.start();
    // for timing
#ifdef WIN32
    ::QueryPerformanceCounter(&li);
#else
    gettimeofday(&tv1,NULL);
#endif
    for (int i=0;i<100;++i)
    {
        p2.process_event(open_close);
        p2.process_event(open_close); 
        p2.process_event(cd_detected);
        p2.process_event(play);      
        p2.process_event(pause); 
        // go back to Playing
        p2.process_event(end_pause); 
        p2.process_event(pause); 
        p2.process_event(stop);  
        // event leading to the same state
        p2.process_event(stop);
        p2.process_event(open_close);
        p2.process_event(open_close);
    }
#ifdef WIN32
    ::QueryPerformanceCounter(&li2);
#else
    gettimeofday(&tv2,NULL);
#endif
#ifdef WIN32
    std::cout << "msm took in s:" << (double)(li2.QuadPart-li.QuadPart)/res.QuadPart <<"\n" <<std::endl;
#else
    std::cout << "msm took in us:" <<  mtime(tv1,tv2) <<"\n" <<std::endl;
#endif
    return 0;
}

