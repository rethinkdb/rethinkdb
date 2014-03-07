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
#include <boost/msm/front/euml/euml.hpp>

#define BOOST_TEST_MODULE MyTest
#include <boost/test/unit_test.hpp>

namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace boost::msm::front::euml;

namespace
{
    // events
    BOOST_MSM_EUML_EVENT(play)
    BOOST_MSM_EUML_EVENT(end_pause)
    BOOST_MSM_EUML_EVENT(stop)
    BOOST_MSM_EUML_EVENT(pause)
    BOOST_MSM_EUML_EVENT(open_close)
    BOOST_MSM_EUML_EVENT(next_song)
    BOOST_MSM_EUML_EVENT(previous_song)
    BOOST_MSM_EUML_EVENT(error_found)
    BOOST_MSM_EUML_EVENT(end_error)
    BOOST_MSM_EUML_EVENT(do_terminate)

    // Flags. Allow information about a property of the current state
    BOOST_MSM_EUML_FLAG(PlayingPaused)
    BOOST_MSM_EUML_FLAG(CDLoaded)
    BOOST_MSM_EUML_FLAG(FirstSongPlaying)

    // A "complicated" event type that carries some data.
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(std::string,cd_name)
    BOOST_MSM_EUML_ATTRIBUTES((attributes_ << cd_name ), cd_detected_attributes)
    BOOST_MSM_EUML_EVENT_WITH_ATTRIBUTES(cd_detected,cd_detected_attributes)

    //states
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(unsigned int,entry_counter)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(unsigned int,exit_counter)

    BOOST_MSM_EUML_STATE(( ++state_(entry_counter),++state_(exit_counter),
                           attributes_ << entry_counter << exit_counter, configure_ << play),Empty)
    BOOST_MSM_EUML_STATE(( ++state_(entry_counter),++state_(exit_counter),
                           attributes_ << entry_counter << exit_counter, configure_<< CDLoaded << play),Open)
    BOOST_MSM_EUML_STATE(( ++state_(entry_counter),++state_(exit_counter),
                           attributes_ << entry_counter << exit_counter, configure_<< CDLoaded),Stopped)
    BOOST_MSM_EUML_STATE(( ++state_(entry_counter),++state_(exit_counter),
                           attributes_ << entry_counter << exit_counter, configure_<< CDLoaded << PlayingPaused),Paused)

    BOOST_MSM_EUML_STATE(( ++state_(entry_counter),++state_(exit_counter),attributes_ << entry_counter << exit_counter),AllOk)
    BOOST_MSM_EUML_TERMINATE_STATE(( ++state_(entry_counter),++state_(exit_counter),attributes_ << entry_counter << exit_counter),
                                    ErrorTerminate)
    BOOST_MSM_EUML_INTERRUPT_STATE(( end_error,++state_(entry_counter),++state_(exit_counter),attributes_ << entry_counter << exit_counter),
                                    ErrorMode)

    // Playing is now a state machine itself.
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(unsigned int,start_next_song_counter)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(unsigned int,start_prev_song_guard_counter)

    BOOST_MSM_EUML_STATE(( ++state_(entry_counter),++state_(exit_counter),
                           attributes_ << entry_counter << exit_counter, configure_<< FirstSongPlaying ),Song1)
    BOOST_MSM_EUML_STATE(( ++state_(entry_counter),++state_(exit_counter),attributes_ << entry_counter << exit_counter),Song2)
    BOOST_MSM_EUML_STATE(( ++state_(entry_counter),++state_(exit_counter),attributes_ << entry_counter << exit_counter),Song3)

    // Playing has a transition table 
   BOOST_MSM_EUML_TRANSITION_TABLE((
        //  +------------------------------------------------------------------------------+
            Song2         == Song1          + next_song                                                         ,
            Song1         == Song2          + previous_song   [True_()] / ++fsm_(start_prev_song_guard_counter) ,
            Song3         == Song2          + next_song       / ++fsm_(start_next_song_counter)                 ,
            Song2         == Song3          + previous_song   [True_()] / ++fsm_(start_prev_song_guard_counter)
        //  +------------------------------------------------------------------------------+
        ),playing_transition_table )

    BOOST_MSM_EUML_DECLARE_STATE_MACHINE( (playing_transition_table, //STT
                                        init_ << Song1, // Init State
                                        ++state_(entry_counter), // Entry
                                        ++state_(exit_counter), // Exit
                                        attributes_ << entry_counter << exit_counter 
                                                    << start_next_song_counter
                                                    << start_prev_song_guard_counter, // Attributes
                                        configure_<< PlayingPaused << CDLoaded //flags
                                        ),Playing_)

    // back-end
    typedef msm::back::state_machine<Playing_> Playing_type;
    Playing_type const Playing;

    //fsm
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(unsigned int,start_playback_counter)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(unsigned int,can_close_drawer_counter)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(unsigned int,report_error_counter)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(unsigned int,report_end_error_counter)
    BOOST_MSM_EUML_ACTION(No_Transition)
    {
        template <class FSM,class Event>
        void operator()(Event const&,FSM&,int)
        {
            BOOST_FAIL("no_transition called!");
        }
    };

    BOOST_MSM_EUML_TRANSITION_TABLE((
          Playing       == Stopped  + play        / ++fsm_(start_playback_counter),
          Playing       == Paused   + end_pause   ,
          //  +------------------------------------------------------------------------------+
          Empty         == Open     + open_close  / ++fsm_(can_close_drawer_counter),
          //  +------------------------------------------------------------------------------+
          Open          == Empty    + open_close  ,
          Open          == Paused   + open_close  ,
          Open          == Stopped  + open_close  ,
          Open          == Playing  + open_close  ,
          //  +------------------------------------------------------------------------------+
          Paused        == Playing  + pause       ,
          //  +------------------------------------------------------------------------------+
          Stopped       == Playing  + stop        ,
          Stopped       == Paused   + stop        ,
          Stopped       == Empty    + cd_detected ,
          Stopped       == Stopped  + stop        ,
          ErrorMode     == AllOk    + error_found / ++fsm_(report_error_counter),
          AllOk         == ErrorMode+ end_error   / ++fsm_(report_end_error_counter),
          ErrorTerminate== AllOk    +do_terminate
          //  +------------------------------------------------------------------------------+
         ),transition_table)

    BOOST_MSM_EUML_DECLARE_STATE_MACHINE(( transition_table, //STT
                                        init_ << Empty << AllOk, // Init State
                                        no_action, // Entry
                                        no_action, // Exit
                                        attributes_ << start_playback_counter << can_close_drawer_counter 
                                                    << report_error_counter << report_end_error_counter, // Attributes
                                        configure_ << no_configure_, // configuration
                                        No_Transition // no_transition handler
                                        ),
                                      player_) //fsm name

    // Pick a back-end
    typedef msm::back::state_machine<player_> player;

    //static char const* const state_names[] = { "Stopped", "Paused","Open", "Empty", "Playing" ,"AllOk","ErrorMode","ErrorTerminate" };

    BOOST_AUTO_TEST_CASE( my_test )
    {
        player p;
        // needed to start the highest-level SM. This will call on_entry and mark the start of the SM
        p.start(); 
        // test deferred event
        // deferred in Empty and Open, will be handled only after event cd_detected
        p.process_event(play);
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 3,"Empty should be active"); //Empty
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Open)&>().get_attribute(exit_counter) == 0,
                            "Open exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Playing_type&>().get_attribute(entry_counter) == 0,"Playing entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Empty)&>().get_attribute(entry_counter) == 1,
                            "Empty entry not called correctly");
        //flags
        BOOST_CHECK_MESSAGE(p.is_flag_active<BOOST_MSM_EUML_FLAG_NAME(CDLoaded)>() == false,"CDLoaded should not be active");

        p.process_event(open_close); 
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 2,"Open should be active"); //Open
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Empty)&>().get_attribute(exit_counter) == 1,
                            "Empty exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Open)&>().get_attribute(entry_counter) == 1,
                            "Open entry not called correctly");

        p.process_event(open_close); 
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 3,"Empty should be active"); //Empty
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Open)&>().get_attribute(exit_counter) == 1,
                            "Open exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Empty)&>().get_attribute(entry_counter) == 2,
                            "Empty entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_attribute(can_close_drawer_counter) == 1,"guard not called correctly");

        //deferred event should have been processed
        p.process_event(cd_detected("louie, louie")); 
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 4,"Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Empty)&>().get_attribute(exit_counter) == 2,
                            "Empty exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Stopped)&>().get_attribute(entry_counter) == 1,
                            "Stopped entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Stopped)&>().get_attribute(exit_counter) == 1,
                            "Stopped exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Playing_type&>().get_attribute(entry_counter) == 1,"Playing entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_attribute(start_playback_counter) == 1,"action not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Playing_type&>().current_state()[0] == 0,"Song1 should be active");
        BOOST_CHECK_MESSAGE(
            p.get_state<Playing_type&>().get_state<BOOST_MSM_EUML_STATE_NAME(Song1)&>().get_attribute(entry_counter) == 1,
            "Song1 entry not called correctly");

        //flags
        BOOST_CHECK_MESSAGE(p.is_flag_active<BOOST_MSM_EUML_FLAG_NAME(PlayingPaused)>() == true,"PlayingPaused should be active");
        BOOST_CHECK_MESSAGE(p.is_flag_active<BOOST_MSM_EUML_FLAG_NAME(FirstSongPlaying)>() == true,"FirstSongPlaying should be active");


        p.process_event(next_song);
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 4,"Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<Playing_type&>().current_state()[0] == 1,"Song2 should be active");
        BOOST_CHECK_MESSAGE(
            p.get_state<Playing_type&>().get_state<BOOST_MSM_EUML_STATE_NAME(Song2)&>().get_attribute(entry_counter) == 1,
            "Song2 entry not called correctly");
        BOOST_CHECK_MESSAGE(
            p.get_state<Playing_type&>().get_state<BOOST_MSM_EUML_STATE_NAME(Song1)&>().get_attribute(exit_counter) == 1,
            "Song1 exit not called correctly");
        BOOST_CHECK_MESSAGE(
            p.get_state<Playing_type&>().get_attribute(start_next_song_counter) == 0,
            "submachine action not called correctly");

        p.process_event(next_song);
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 4,"Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<Playing_type&>().current_state()[0] == 2,"Song3 should be active");
        BOOST_CHECK_MESSAGE(
            p.get_state<Playing_type&>().get_state<BOOST_MSM_EUML_STATE_NAME(Song3)&>().get_attribute(entry_counter) == 1,
            "Song3 entry not called correctly");
        BOOST_CHECK_MESSAGE(
            p.get_state<Playing_type&>().get_state<BOOST_MSM_EUML_STATE_NAME(Song2)&>().get_attribute(exit_counter) == 1,
            "Song2 exit not called correctly");
        BOOST_CHECK_MESSAGE(
            p.get_state<Playing_type&>().get_attribute(start_next_song_counter) == 1,
            "submachine action not called correctly");

        p.process_event(previous_song);
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 4,"Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<Playing_type&>().current_state()[0] == 1,"Song2 should be active");
        BOOST_CHECK_MESSAGE(
            p.get_state<Playing_type&>().get_state<BOOST_MSM_EUML_STATE_NAME(Song2)&>().get_attribute(entry_counter) == 2,
            "Song2 entry not called correctly");
        BOOST_CHECK_MESSAGE(
            p.get_state<Playing_type&>().get_state<BOOST_MSM_EUML_STATE_NAME(Song3)&>().get_attribute(exit_counter) == 1,
            "Song3 exit not called correctly");
        BOOST_CHECK_MESSAGE(
            p.get_state<Playing_type&>().get_attribute(start_prev_song_guard_counter) == 1,
            "submachine guard not called correctly");
        //flags
        BOOST_CHECK_MESSAGE(p.is_flag_active<BOOST_MSM_EUML_FLAG_NAME(PlayingPaused)>() == true,"PlayingPaused should be active");
        BOOST_CHECK_MESSAGE(p.is_flag_active<BOOST_MSM_EUML_FLAG_NAME(FirstSongPlaying)>() == false,"FirstSongPlaying should not be active");

        p.process_event(pause());
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 1,"Paused should be active"); //Paused
        BOOST_CHECK_MESSAGE(p.get_state<Playing_type&>().get_attribute(exit_counter) == 1,"Playing exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Paused)&>().get_attribute(entry_counter) == 1,
                            "Paused entry not called correctly");
        //flags
        BOOST_CHECK_MESSAGE(p.is_flag_active<BOOST_MSM_EUML_FLAG_NAME(PlayingPaused)>() == true,"PlayingPaused should be active");

        // go back to Playing
        p.process_event(end_pause);  
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 4,"Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Paused)&>().get_attribute(exit_counter) == 1,
                            "Paused exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Playing_type&>().get_attribute(entry_counter) == 2,"Playing entry not called correctly");

        p.process_event(pause); 
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 1,"Paused should be active"); //Paused
        BOOST_CHECK_MESSAGE(p.get_state<Playing_type&>().get_attribute(exit_counter) == 2,"Playing exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Paused)&>().get_attribute(entry_counter) == 2,
                            "Paused entry not called correctly");

        p.process_event(stop);  
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 0,"Stopped should be active"); //Stopped
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Paused)&>().get_attribute(exit_counter) == 2,
                            "Paused exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Stopped)&>().get_attribute(entry_counter) == 2,
                            "Stopped entry not called correctly");
        //flags
        BOOST_CHECK_MESSAGE(p.is_flag_active<BOOST_MSM_EUML_FLAG_NAME(PlayingPaused)>() == false,"PlayingPaused should not be active");
        BOOST_CHECK_MESSAGE(p.is_flag_active<BOOST_MSM_EUML_FLAG_NAME(CDLoaded)>() == true,"CDLoaded should be active");
        //BOOST_CHECK_MESSAGE(p.is_flag_active<BOOST_MSM_EUML_FLAG_NAME(CDLoaded),player::Flag_AND>() == false,"CDLoaded with AND should not be active");

        p.process_event(stop);  
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 0,"Stopped should be active"); //Stopped
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Stopped)&>().get_attribute(exit_counter) == 2,
                            "Stopped exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Stopped)&>().get_attribute(entry_counter) == 3,
                            "Stopped entry not called correctly");

        //test interrupt
        BOOST_CHECK_MESSAGE(p.current_state()[1] == 5,"AllOk should be active"); //AllOk
        p.process_event(error_found);
        BOOST_CHECK_MESSAGE(p.current_state()[1] == 6,"ErrorMode should be active"); //ErrorMode
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(AllOk)&>().get_attribute(exit_counter) == 1,
                            "AllOk exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(ErrorMode)&>().get_attribute(entry_counter) == 1,
                            "ErrorMode entry not called correctly");

        // try generating more events
        p.process_event(play);
        BOOST_CHECK_MESSAGE(p.current_state()[1] == 6,"ErrorMode should be active"); //ErrorMode
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(AllOk)&>().get_attribute(exit_counter) == 1,
                            "AllOk exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(ErrorMode)&>().get_attribute(entry_counter) == 1,
                            "ErrorMode entry not called correctly");
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 0,"Stopped should be active"); //Stopped
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Stopped)&>().get_attribute(exit_counter) == 2,
                            "Stopped exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Stopped)&>().get_attribute(entry_counter) == 3,
                            "Stopped entry not called correctly");

        p.process_event(end_error);
        BOOST_CHECK_MESSAGE(p.current_state()[1] == 5,"AllOk should be active"); //AllOk
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(ErrorMode)&>().get_attribute(exit_counter) == 1,
                            "ErrorMode exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(AllOk)&>().get_attribute(entry_counter) == 2,
                            "AllOk entry not called correctly");

        p.process_event(play);
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 4,"Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Stopped)&>().get_attribute(exit_counter) == 3,
                            "Stopped exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<Playing_type&>().get_attribute(entry_counter) == 3,"Playing entry not called correctly");

        //test terminate
        BOOST_CHECK_MESSAGE(p.current_state()[1] == 5,"AllOk should be active"); //AllOk
        p.process_event(do_terminate);
        BOOST_CHECK_MESSAGE(p.current_state()[1] == 7,"ErrorTerminate should be active"); //ErrorTerminate
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(AllOk)&>().get_attribute(exit_counter) == 2,
                            "AllOk exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(ErrorTerminate)&>().get_attribute(entry_counter) == 1,
                            "ErrorTerminate entry not called correctly");

        // try generating more events
        p.process_event(stop);
        BOOST_CHECK_MESSAGE(p.current_state()[1] == 7,"ErrorTerminate should be active"); //ErrorTerminate
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(ErrorTerminate)&>().get_attribute(exit_counter) == 0,
                            "ErrorTerminate exit not called correctly");
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 4,"Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<Playing_type&>().get_attribute(exit_counter) == 2,"Playing exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Stopped)&>().get_attribute(entry_counter) == 3,
                            "Stopped entry not called correctly");

        p.process_event(end_error());
        BOOST_CHECK_MESSAGE(p.current_state()[1] == 7,"ErrorTerminate should be active"); //ErrorTerminate
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 4,"Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<Playing_type&>().get_attribute(exit_counter) == 2,"Playing exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Stopped)&>().get_attribute(entry_counter) == 3,
                            "Stopped entry not called correctly");

        p.process_event(stop());
        BOOST_CHECK_MESSAGE(p.current_state()[1] == 7,"ErrorTerminate should be active"); //ErrorTerminate
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 4,"Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<Playing_type&>().get_attribute(exit_counter) == 2,"Playing exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML_STATE_NAME(Stopped)&>().get_attribute(entry_counter) == 3,
                            "Stopped entry not called correctly");

    }
}

