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

// entry/exit/action/guard logging functors
#include "logging_functors.h"

namespace  // Concrete FSM implementation
{
    // events
    BOOST_MSM_EUML_EVENT(play)
    BOOST_MSM_EUML_EVENT(end_pause)
    BOOST_MSM_EUML_EVENT(stop)
    BOOST_MSM_EUML_EVENT(pause)
    BOOST_MSM_EUML_EVENT(open_close)
    BOOST_MSM_EUML_EVENT(next_song)
    BOOST_MSM_EUML_EVENT(previous_song)
    BOOST_MSM_EUML_EVENT(end_error)
    BOOST_MSM_EUML_EVENT(error_found)

    // A "complicated" event type that carries some data.
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(std::string,cd_name)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(DiskTypeEnum,cd_type)
    BOOST_MSM_EUML_ATTRIBUTES((attributes_ << cd_name << cd_type ), cd_detected_attributes)
    BOOST_MSM_EUML_EVENT_WITH_ATTRIBUTES(cd_detected,cd_detected_attributes)

    // Flags. Allow information about a property of the current state
    BOOST_MSM_EUML_FLAG(PlayingPaused)
    BOOST_MSM_EUML_FLAG(CDLoaded)
    BOOST_MSM_EUML_FLAG(FirstSongPlaying)

    // Concrete FSM implementation 
    // The list of FSM states
    BOOST_MSM_EUML_STATE((  Empty_Entry,
                            Empty_Exit,
                            attributes_ << no_attributes_,
                            configure_ << no_configure_
                          ),
                          Empty)

    BOOST_MSM_EUML_STATE((  Open_Entry,
                            Open_Exit,
                            attributes_ << no_attributes_,
                            configure_<< CDLoaded // flag state with CDLoaded
                          ),
                          Open)

    BOOST_MSM_EUML_STATE((  Stopped_Entry,
                            Stopped_Exit,
                            attributes_ << no_attributes_,
                            configure_<< CDLoaded // flag state with CDLoaded
                          ),
                          Stopped)

    // state not defining any entry or exit
    BOOST_MSM_EUML_STATE((  no_action,
                            no_action,
                            attributes_ << no_attributes_,
                            configure_<< PlayingPaused << CDLoaded // flag state with CDLoaded and PlayingPaused
                          ),
                          Paused)

    BOOST_MSM_EUML_STATE(( AllOk_Entry,AllOk_Exit ),AllOk)

    // a terminate state 
    //BOOST_MSM_EUML_TERMINATE_STATE(( ErrorMode_Entry,ErrorMode_Exit ),ErrorMode)
    // or as an interrupt state
    BOOST_MSM_EUML_INTERRUPT_STATE(( end_error,ErrorMode_Entry,ErrorMode_Exit ),ErrorMode)

    // Playing is now a state machine itself.

    // It has 3 substates
    BOOST_MSM_EUML_STATE((  Song1_Entry,
    Song1_Exit,
    attributes_ << no_attributes_,
    configure_<< FirstSongPlaying ),Song1)

    BOOST_MSM_EUML_STATE(( Song2_Entry,Song2_Exit ),Song2)
    BOOST_MSM_EUML_STATE(( Song3_Entry,Song3_Exit ),Song3)


    // Playing has a transition table 
    BOOST_MSM_EUML_TRANSITION_TABLE((
        //  +------------------------------------------------------------------------------+
        Song2  == Song1 + next_song       / start_next_song,
        Song1  == Song2 + previous_song   / start_prev_song,
        Song3  == Song2 + next_song       / start_next_song,
        Song2  == Song3 + previous_song   / start_prev_song
        //  +------------------------------------------------------------------------------+
        ),playing_transition_table )

    BOOST_MSM_EUML_DECLARE_STATE_MACHINE( (playing_transition_table, //STT
                                        init_ << Song1, // Init State
                                        no_action, // entry
                                        no_action, // exit
                                        attributes_ << no_attributes_, //attributes
                                        configure_<< PlayingPaused << CDLoaded //flags
                                        ),Playing_)

    // choice of back-end
    typedef msm::back::state_machine<Playing_> Playing_type;
    Playing_type const Playing;

    // guard conditions
    BOOST_MSM_EUML_ACTION(good_disk_format)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        bool operator()(EVT const& evt,FSM&,SourceState& ,TargetState& )
        {
            // to test a guard condition, let's say we understand only CDs, not DVD
            if (evt.get_attribute(cd_type)!=DISK_CD)
            {
                std::cout << "wrong disk, sorry" << std::endl;
                // just for logging, does not block any transition
                return true;
            }
            std::cout << "good disk" << std::endl;
            return true;
        }
    };
    // replaces the old transition table
    BOOST_MSM_EUML_TRANSITION_TABLE((
          Playing   == Stopped  + play        / start_playback                  ,
          Playing   == Paused   + end_pause   / resume_playback                 ,
          //  +------------------------------------------------------------------------------+
          Empty     == Open     + open_close  / close_drawer,
          // we now defer using the defer_ function. This will need deferred_events as config (see below)
          Empty                 + play        / defer_                          ,
          //  +------------------------------------------------------------------------------+
          Open      == Empty    + open_close  / open_drawer                     ,
          Open      == Paused   + open_close  / stop_and_open                   ,
          Open      == Stopped  + open_close  / open_drawer                     ,
          Open      == Playing  + open_close  / stop_and_open                   ,
          // we now defer using the defer_ function. This will need deferred_events as config (see below)
          Open                  + play        / defer_                          ,
          //  +------------------------------------------------------------------------------+
          Paused    == Playing  + pause       / pause_playback                  ,
          //  +------------------------------------------------------------------------------+
          Stopped   == Playing  + stop        / stop_playback                   ,
          Stopped   == Paused   + stop        / stop_playback                   ,
          Stopped   == Empty    + cd_detected [good_disk_format&&
                                                     (event_(cd_type)==Int_<DISK_CD>())] 
                                                    / (store_cd_info,process_(play)),
          Stopped   == Stopped  + stop                                          ,
          ErrorMode == AllOk    + error_found / report_error                    ,
          AllOk     == ErrorMode+ end_error   / report_end_error
          //  +------------------------------------------------------------------------------+
          ),transition_table)

    // create a state machine "on the fly"
    BOOST_MSM_EUML_DECLARE_STATE_MACHINE(( transition_table, //STT
                                        init_ << Empty << AllOk, // Init State
                                        no_action, // Entry
                                        no_action, // Exit
                                        attributes_ << no_attributes_, // Attributes
                                        configure_ << deferred_events, // configuration
                                        Log_No_Transition // no_transition handler
                                        ),
                                      player_) //fsm name

    // choice of back-end
    typedef msm::back::state_machine<player_> player;

    //
    // Testing utilities.
    //
    static char const* const state_names[] = { "Stopped", "Paused", "Open", "Empty", "Playing","AllOk","ErrorMode" };
    void pstate(player const& p)
    {
        // we have now several active states, which we show
        for (unsigned int i=0;i<player::nr_regions::value;++i)
        {
            std::cout << " -> " << state_names[p.current_state()[i]] << std::endl;
        }
    }

    void test()
    {        
        player p;
        // needed to start the highest-level SM. This will call on_entry and mark the start of the SM
        p.start(); 

        // tests some flags
        std::cout << "CDLoaded active:" << std::boolalpha 
                  << p.is_flag_active<BOOST_MSM_EUML_FLAG_NAME(CDLoaded)>() << std::endl; //=> false (no CD yet)
        // go to Open, call on_exit on Empty, then action, then on_entry on Open
        p.process_event(open_close); pstate(p);
        p.process_event(open_close); pstate(p);
        // will be rejected, wrong disk type
        p.process_event(
            cd_detected("louie, louie",DISK_DVD)); pstate(p);
        p.process_event(
            cd_detected("louie, louie",DISK_CD)); pstate(p);
        // no need to call play as the previous event does it in its action method
        //p.process_event(play);

        // at this point, Play is active 
        p.process_event(pause); pstate(p);
        std::cout << "PlayingPaused active:" << std::boolalpha 
                  << p.is_flag_active<BOOST_MSM_EUML_FLAG_NAME(PlayingPaused)>() << std::endl;//=> true

        // go back to Playing
        p.process_event(end_pause);  pstate(p);
        p.process_event(pause); pstate(p);
        p.process_event(stop);  pstate(p);
        std::cout << "PlayingPaused active:" << std::boolalpha 
                  << p.is_flag_active<BOOST_MSM_EUML_FLAG_NAME(PlayingPaused)>() << std::endl;//=> false
        std::cout << "CDLoaded active:" << std::boolalpha 
                  << p.is_flag_active<BOOST_MSM_EUML_FLAG_NAME(CDLoaded)>() << std::endl;//=> true
        // by default, the flags are OR'ed but you can also use AND. Then the flag must be present in 
        // all of the active states
        std::cout << "CDLoaded active with AND:" << std::boolalpha 
                  << p.is_flag_active<BOOST_MSM_EUML_FLAG_NAME(CDLoaded),player::Flag_AND>() << std::endl;//=> false

        // event leading to the same state
        // no action method called as none is defined in the transition table
        p.process_event(stop);  pstate(p);

        // event leading to a terminal/interrupt state
        p.process_event(error_found);  pstate(p);
        // try generating more events
        std::cout << "Trying to generate another event" << std::endl; // will not work, fsm is terminated or interrupted
        p.process_event(play);pstate(p);
        std::cout << "Trying to end the error" << std::endl; // will work only if ErrorMode is interrupt state
        p.process_event(end_error);pstate(p);
        std::cout << "Trying to generate another event" << std::endl; // will work only if ErrorMode is interrupt state
        p.process_event(play);pstate(p);
    }
}

int main()
{
    test();
    return 0;
}
