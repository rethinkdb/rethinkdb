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
    struct play {};
    struct end_pause {};
    struct stop {};
    struct pause {};
    struct open_close {};
    struct NextSong {};
    struct PreviousSong {};
    struct error_found {};
    struct end_error {};
    struct do_terminate {};

    // Flags. Allow information about a property of the current state
    struct PlayingPaused{};
    struct CDLoaded {};
    struct FirstSongPlaying {};

    // A "complicated" event type that carries some data.
    struct cd_detected
    {
        cd_detected(std::string name)
            : name(name)
        {}

        std::string name;
    };

    // front-end: define the FSM structure 
    struct player_ : public msm::front::state_machine_def<player_>
    {
        unsigned int start_playback_counter;
        unsigned int can_close_drawer_counter;
        unsigned int report_error_counter;
        unsigned int report_end_error_counter;

        player_():
        start_playback_counter(0),
        can_close_drawer_counter(0),
        report_error_counter(0),
        report_end_error_counter(0)
        {}
        // The list of FSM states
        struct Empty : public msm::front::state<> 
        {
            typedef mpl::vector<play> deferred_events;
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {++entry_counter;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {++exit_counter;}
            int entry_counter;
            int exit_counter;
        };
        struct Open : public msm::front::state<> 
        { 
            typedef mpl::vector<play> deferred_events;
            typedef mpl::vector1<CDLoaded>      flag_list;
            
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {++entry_counter;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {++exit_counter;}
            int entry_counter;
            int exit_counter;
        };

        struct Stopped : public msm::front::state<> 
        { 
            typedef mpl::vector1<CDLoaded>      flag_list;

            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {++entry_counter;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {++exit_counter;}
            int entry_counter;
            int exit_counter;
        };

        // the player state machine contains a state which is himself a state machine
        // as you see, no need to declare it anywhere so Playing can be developed separately
        // by another team in another module. For simplicity I just declare it inside player
        struct Playing_ : public msm::front::state_machine_def<Playing_>
        {
            // when playing, the CD is loaded and we are in either pause or playing (duh)
            typedef mpl::vector2<PlayingPaused,CDLoaded>        flag_list;

            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {++entry_counter;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {++exit_counter;}
            int entry_counter;
            int exit_counter;
            unsigned int start_next_song_counter;
            unsigned int start_prev_song_guard_counter;

            Playing_():
            start_next_song_counter(0),
            start_prev_song_guard_counter(0)
            {}

            // The list of FSM states
            struct Song1 : public msm::front::state<>
            {
                typedef mpl::vector1<FirstSongPlaying>      flag_list;

                template <class Event,class FSM>
                void on_entry(Event const&,FSM& ) {++entry_counter;}
                template <class Event,class FSM>
                void on_exit(Event const&,FSM& ) {++exit_counter;}
                int entry_counter;
                int exit_counter;
            };
            struct Song2 : public msm::front::state<>
            { 
                template <class Event,class FSM>
                void on_entry(Event const&,FSM& ) {++entry_counter;}
                template <class Event,class FSM>
                void on_exit(Event const&,FSM& ) {++exit_counter;}
                int entry_counter;
                int exit_counter;
            };
            struct Song3 : public msm::front::state<>
            { 
                template <class Event,class FSM>
                void on_entry(Event const&,FSM& ) {++entry_counter;}
                template <class Event,class FSM>
                void on_exit(Event const&,FSM& ) {++exit_counter;}
                int entry_counter;
                int exit_counter;
            };
            // the initial state. Must be defined
            typedef Song1 initial_state;
            // transition actions
            void start_next_song(NextSong const&)       {++start_next_song_counter; }
            void start_prev_song(PreviousSong const&)       {  }
            // guard conditions
            bool start_prev_song_guard(PreviousSong const&)       {++start_prev_song_guard_counter;return true; }

            typedef Playing_ pl; // makes transition table cleaner
            // Transition table for Playing
            struct transition_table : mpl::vector4<
                //      Start     Event         Next      Action               Guard
                //    +---------+-------------+---------+---------------------+----------------------+
                 _row < Song1   , NextSong    , Song2                                                >,
                  row < Song2   , PreviousSong, Song1   , &pl::start_prev_song,&pl::start_prev_song_guard>,
                a_row < Song2   , NextSong    , Song3   , &pl::start_next_song                       >,
                g_row < Song3   , PreviousSong, Song2                         ,&pl::start_prev_song_guard>
                //    +---------+-------------+---------+---------------------+----------------------+
            > {};
            // Replaces the default no-transition response.
            template <class FSM,class Event>
            void no_transition(Event const&, FSM&,int)
            {
                BOOST_FAIL("no_transition called!");
            }
        };
        // back-end
        typedef msm::back::state_machine<Playing_> Playing;

        // state not defining any entry or exit
        struct Paused : public msm::front::state<>
        {
            typedef mpl::vector2<PlayingPaused,CDLoaded>        flag_list;

            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {++entry_counter;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {++exit_counter;}
            int entry_counter;
            int exit_counter;
        };
        struct AllOk : public msm::front::state<>
        {
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {++entry_counter;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {++exit_counter;}
            int entry_counter;
            int exit_counter;
        };
        // this state is also made terminal so that all the events are blocked
        struct ErrorMode :  //public msm::front::terminate_state<> // ErrorMode terminates the state machine
            public msm::front::interrupt_state<end_error>   // ErroMode just interrupts. Will resume if
                                                            // the event end_error is generated
        {
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {++entry_counter;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {++exit_counter;}
            int entry_counter;
            int exit_counter;
        };
        struct ErrorTerminate :  public msm::front::terminate_state<> // terminates the state machine
        {
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {++entry_counter;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {++exit_counter;}
            int entry_counter;
            int exit_counter;
        };
        // the initial state of the player SM. Must be defined
        typedef mpl::vector<Empty,AllOk> initial_state;

        // transition actions
        void start_playback(play const&)       {++start_playback_counter; }
        void open_drawer(open_close const&)    {  }
        void store_cd_info(cd_detected const&) {  }
        void stop_playback(stop const&)        {  }
        void pause_playback(pause const&)      {  }
        void resume_playback(end_pause const&)      {  }
        void stop_and_open(open_close const&)  {  }
        void stopped_again(stop const&){}
        void report_error(error_found const&) {++report_error_counter;}
        void report_end_error(end_error const&) {++report_end_error_counter;}

        //guards
        bool can_close_drawer(open_close const&)   
        {
            ++can_close_drawer_counter;
            return true;
        }
        typedef player_ p; // makes transition table cleaner

        // Transition table for player
        struct transition_table : mpl::vector<
            //      Start     Event         Next      Action               Guard
            //    +---------+-------------+---------+---------------------+----------------------+
            a_row < Stopped , play        , Playing , &p::start_playback                         >,
            a_row < Stopped , open_close  , Open    , &p::open_drawer                            >,
             _row < Stopped , stop        , Stopped                                              >,
            //  +---------+-------------+---------+---------------------+----------------------+
            g_row < Open    , open_close  , Empty   ,                      &p::can_close_drawer  >,
            //  +---------+-------------+---------+---------------------+----------------------+
            a_row < Empty   , open_close  , Open    , &p::open_drawer                            >,
            a_row < Empty   , cd_detected , Stopped , &p::store_cd_info                          >,
            //  +---------+-------------+---------+---------------------+----------------------+
            a_row < Playing , stop        , Stopped , &p::stop_playback                          >,
            a_row < Playing , pause       , Paused  , &p::pause_playback                         >,
            a_row < Playing , open_close  , Open    , &p::stop_and_open                          >,
            //  +---------+-------------+---------+---------------------+----------------------+
            a_row < Paused  , end_pause   , Playing , &p::resume_playback                        >,
            a_row < Paused  , stop        , Stopped , &p::stop_playback                          >,
            a_row < Paused  , open_close  , Open    , &p::stop_and_open                          >,
            //    +---------+-------------+---------+---------------------+----------------------+
            a_row < AllOk   , error_found ,ErrorMode, &p::report_error                           >,
            a_row <ErrorMode, end_error   ,AllOk    , &p::report_end_error                       >,
             _row < AllOk   , do_terminate,ErrorTerminate                                        >
            //    +---------+-------------+---------+---------------------+----------------------+
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
            fsm.template get_state<player_::Stopped&>().entry_counter=0;
            fsm.template get_state<player_::Stopped&>().exit_counter=0;
            fsm.template get_state<player_::Open&>().entry_counter=0;
            fsm.template get_state<player_::Open&>().exit_counter=0;
            fsm.template get_state<player_::Empty&>().entry_counter=0;
            fsm.template get_state<player_::Empty&>().exit_counter=0;
            fsm.template get_state<player_::Playing&>().entry_counter=0;
            fsm.template get_state<player_::Playing&>().exit_counter=0;
            fsm.template get_state<player_::Playing&>().template get_state<player_::Playing::Song1&>().entry_counter=0;
            fsm.template get_state<player_::Playing&>().template get_state<player_::Playing::Song1&>().exit_counter=0;
            fsm.template get_state<player_::Playing&>().template get_state<player_::Playing::Song2&>().entry_counter=0;
            fsm.template get_state<player_::Playing&>().template get_state<player_::Playing::Song2&>().exit_counter=0;
            fsm.template get_state<player_::Playing&>().template get_state<player_::Playing::Song3&>().entry_counter=0;
            fsm.template get_state<player_::Playing&>().template get_state<player_::Playing::Song3&>().exit_counter=0;
            fsm.template get_state<player_::Paused&>().entry_counter=0;
            fsm.template get_state<player_::Paused&>().exit_counter=0;
            fsm.template get_state<player_::AllOk&>().entry_counter=0;
            fsm.template get_state<player_::AllOk&>().exit_counter=0;
            fsm.template get_state<player_::ErrorMode&>().entry_counter=0;
            fsm.template get_state<player_::ErrorMode&>().exit_counter=0;
            fsm.template get_state<player_::ErrorTerminate&>().entry_counter=0;
            fsm.template get_state<player_::ErrorTerminate&>().exit_counter=0;
        }
    };
    // Pick a back-end
    typedef msm::back::state_machine<player_> player;

    //static char const* const state_names[] = { "Stopped", "Open", "Empty", "Playing", "Paused","AllOk","ErrorMode" };

    BOOST_AUTO_TEST_CASE( my_test )
    {
        player p;
        // needed to start the highest-level SM. This will call on_entry and mark the start of the SM
        p.start(); 
        // test deferred event
        // deferred in Empty and Open, will be handled only after event cd_detected
        p.process_event(play());
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 2,"Empty should be active"); //Empty
        BOOST_CHECK_MESSAGE(p.get_state<player_::Open&>().exit_counter == 0,"Open exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Playing&>().entry_counter == 0,"Playing entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Empty&>().entry_counter == 1,"Empty entry not called correctly");
        //flags
        BOOST_CHECK_MESSAGE(p.is_flag_active<CDLoaded>() == false,"CDLoaded should not be active");

        p.process_event(open_close()); 
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 1,"Open should be active"); //Open
        BOOST_CHECK_MESSAGE(p.get_state<player_::Empty&>().exit_counter == 1,"Empty exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Open&>().entry_counter == 1,"Open entry not called correctly");

        p.process_event(open_close()); 
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 2,"Empty should be active"); //Empty
        BOOST_CHECK_MESSAGE(p.get_state<player_::Open&>().exit_counter == 1,"Open exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Empty&>().entry_counter == 2,"Empty entry not called correctly");
        BOOST_CHECK_MESSAGE(p.can_close_drawer_counter == 1,"guard not called correctly");

        //deferred event should have been processed
        p.process_event(cd_detected("louie, louie")); 
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 3,"Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<player_::Empty&>().exit_counter == 2,"Empty exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Stopped&>().entry_counter == 1,"Stopped entry not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Stopped&>().exit_counter == 1,"Stopped exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Playing&>().entry_counter == 1,"Playing entry not called correctly");
        BOOST_CHECK_MESSAGE(p.start_playback_counter == 1,"action not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Playing&>().current_state()[0] == 0,"Song1 should be active");
        BOOST_CHECK_MESSAGE(
            p.get_state<player_::Playing&>().get_state<player_::Playing::Song1&>().entry_counter == 1,
            "Song1 entry not called correctly");

        //flags
        BOOST_CHECK_MESSAGE(p.is_flag_active<PlayingPaused>() == true,"PlayingPaused should be active");
        BOOST_CHECK_MESSAGE(p.is_flag_active<FirstSongPlaying>() == true,"FirstSongPlaying should be active");


        p.process_event(NextSong());
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 3,"Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<player_::Playing&>().current_state()[0] == 1,"Song2 should be active");
        BOOST_CHECK_MESSAGE(
            p.get_state<player_::Playing&>().get_state<player_::Playing::Song2&>().entry_counter == 1,
            "Song2 entry not called correctly");
        BOOST_CHECK_MESSAGE(
            p.get_state<player_::Playing&>().get_state<player_::Playing::Song1&>().exit_counter == 1,
            "Song1 exit not called correctly");
        BOOST_CHECK_MESSAGE(
            p.get_state<player_::Playing&>().start_next_song_counter == 0,
            "submachine action not called correctly");

        p.process_event(NextSong());
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 3,"Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<player_::Playing&>().current_state()[0] == 2,"Song3 should be active");
        BOOST_CHECK_MESSAGE(
            p.get_state<player_::Playing&>().get_state<player_::Playing::Song3&>().entry_counter == 1,
            "Song3 entry not called correctly");
        BOOST_CHECK_MESSAGE(
            p.get_state<player_::Playing&>().get_state<player_::Playing::Song2&>().exit_counter == 1,
            "Song2 exit not called correctly");
        BOOST_CHECK_MESSAGE(
            p.get_state<player_::Playing&>().start_next_song_counter == 1,
            "submachine action not called correctly");

        p.process_event(PreviousSong());
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 3,"Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<player_::Playing&>().current_state()[0] == 1,"Song2 should be active");
        BOOST_CHECK_MESSAGE(
            p.get_state<player_::Playing&>().get_state<player_::Playing::Song2&>().entry_counter == 2,
            "Song2 entry not called correctly");
        BOOST_CHECK_MESSAGE(
            p.get_state<player_::Playing&>().get_state<player_::Playing::Song3&>().exit_counter == 1,
            "Song3 exit not called correctly");
        BOOST_CHECK_MESSAGE(
            p.get_state<player_::Playing&>().start_prev_song_guard_counter == 1,
            "submachine guard not called correctly");
        //flags
        BOOST_CHECK_MESSAGE(p.is_flag_active<PlayingPaused>() == true,"PlayingPaused should be active");
        BOOST_CHECK_MESSAGE(p.is_flag_active<FirstSongPlaying>() == false,"FirstSongPlaying should not be active");

        p.process_event(pause());
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 4,"Paused should be active"); //Paused
        BOOST_CHECK_MESSAGE(p.get_state<player_::Playing&>().exit_counter == 1,"Playing exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Paused&>().entry_counter == 1,"Paused entry not called correctly");
        //flags
        BOOST_CHECK_MESSAGE(p.is_flag_active<PlayingPaused>() == true,"PlayingPaused should be active");

        // go back to Playing
        p.process_event(end_pause());  
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 3,"Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<player_::Paused&>().exit_counter == 1,"Paused exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Playing&>().entry_counter == 2,"Playing entry not called correctly");

        p.process_event(pause()); 
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 4,"Paused should be active"); //Paused
        BOOST_CHECK_MESSAGE(p.get_state<player_::Playing&>().exit_counter == 2,"Playing exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Paused&>().entry_counter == 2,"Paused entry not called correctly");

        p.process_event(stop());  
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 0,"Stopped should be active"); //Stopped
        BOOST_CHECK_MESSAGE(p.get_state<player_::Paused&>().exit_counter == 2,"Paused exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Stopped&>().entry_counter == 2,"Stopped entry not called correctly");
        //flags
        BOOST_CHECK_MESSAGE(p.is_flag_active<PlayingPaused>() == false,"PlayingPaused should not be active");
        BOOST_CHECK_MESSAGE(p.is_flag_active<CDLoaded>() == true,"CDLoaded should be active");
        //BOOST_CHECK_MESSAGE(p.is_flag_active<CDLoaded,player::Flag_AND>() == false,"CDLoaded with AND should not be active");

        p.process_event(stop());  
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 0,"Stopped should be active"); //Stopped
        BOOST_CHECK_MESSAGE(p.get_state<player_::Stopped&>().exit_counter == 2,"Stopped exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Stopped&>().entry_counter == 3,"Stopped entry not called correctly");

        //test interrupt
        BOOST_CHECK_MESSAGE(p.current_state()[1] == 5,"AllOk should be active"); //AllOk
        p.process_event(error_found());
        BOOST_CHECK_MESSAGE(p.current_state()[1] == 6,"ErrorMode should be active"); //ErrorMode
        BOOST_CHECK_MESSAGE(p.get_state<player_::AllOk&>().exit_counter == 1,"AllOk exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::ErrorMode&>().entry_counter == 1,"ErrorMode entry not called correctly");

        // try generating more events
        p.process_event(play());
        BOOST_CHECK_MESSAGE(p.current_state()[1] == 6,"ErrorMode should be active"); //ErrorMode
        BOOST_CHECK_MESSAGE(p.get_state<player_::AllOk&>().exit_counter == 1,"AllOk exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::ErrorMode&>().entry_counter == 1,"ErrorMode entry not called correctly");
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 0,"Stopped should be active"); //Stopped
        BOOST_CHECK_MESSAGE(p.get_state<player_::Stopped&>().exit_counter == 2,"Stopped exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Stopped&>().entry_counter == 3,"Stopped entry not called correctly");

        p.process_event(end_error());
        BOOST_CHECK_MESSAGE(p.current_state()[1] == 5,"AllOk should be active"); //AllOk
        BOOST_CHECK_MESSAGE(p.get_state<player_::ErrorMode&>().exit_counter == 1,"ErrorMode exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::AllOk&>().entry_counter == 2,"AllOk entry not called correctly");

        p.process_event(play());
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 3,"Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<player_::Stopped&>().exit_counter == 3,"Stopped exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Playing&>().entry_counter == 3,"Playing entry not called correctly");

        //test terminate
        BOOST_CHECK_MESSAGE(p.current_state()[1] == 5,"AllOk should be active"); //AllOk
        p.process_event(do_terminate());
        BOOST_CHECK_MESSAGE(p.current_state()[1] == 7,"ErrorTerminate should be active"); //ErrorTerminate
        BOOST_CHECK_MESSAGE(p.get_state<player_::AllOk&>().exit_counter == 2,"AllOk exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::ErrorTerminate&>().entry_counter == 1,"ErrorTerminate entry not called correctly");

        // try generating more events
        p.process_event(stop());
        BOOST_CHECK_MESSAGE(p.current_state()[1] == 7,"ErrorTerminate should be active"); //ErrorTerminate
        BOOST_CHECK_MESSAGE(p.get_state<player_::ErrorTerminate&>().exit_counter == 0,"ErrorTerminate exit not called correctly");
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 3,"Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<player_::Playing&>().exit_counter == 2,"Playing exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Stopped&>().entry_counter == 3,"Stopped entry not called correctly");

        p.process_event(end_error());
        BOOST_CHECK_MESSAGE(p.current_state()[1] == 7,"ErrorTerminate should be active"); //ErrorTerminate
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 3,"Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<player_::Playing&>().exit_counter == 2,"Playing exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Stopped&>().entry_counter == 3,"Stopped entry not called correctly");

        p.process_event(stop());
        BOOST_CHECK_MESSAGE(p.current_state()[1] == 7,"ErrorTerminate should be active"); //ErrorTerminate
        BOOST_CHECK_MESSAGE(p.current_state()[0] == 3,"Playing should be active"); //Playing
        BOOST_CHECK_MESSAGE(p.get_state<player_::Playing&>().exit_counter == 2,"Playing exit not called correctly");
        BOOST_CHECK_MESSAGE(p.get_state<player_::Stopped&>().entry_counter == 3,"Stopped entry not called correctly");

    }
}

