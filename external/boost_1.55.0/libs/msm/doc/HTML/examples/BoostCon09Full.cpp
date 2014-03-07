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
#include <boost/mpl/vector/vector50.hpp>
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/back/tools.hpp>

namespace msm = boost::msm;
namespace mpl = boost::mpl;

namespace  // Concrete FSM implementation
{
    // events
    struct play {};
    struct end_pause {};
    struct stop {};
    struct pause {};
    struct open_close {};
    struct NextSong {};
    // event which every other event can convert to
    struct AllSongsPlayed 
    {   
        template <class Event>
        AllSongsPlayed(Event const&){}
    };
    struct PreviousSong {};
    struct error_found {};
    struct end_error {};

    // Flags. Allow information about a property of the current state
    struct CDLoaded {};
    struct FirstSongPlaying {};

    // A "complicated" event type that carries some data.
    enum DiskTypeEnum
    {
        DISK_CD=0,
        DISK_DVD=1
    };
    struct cd_detected
    {
        cd_detected(DiskTypeEnum diskType): disc_type(diskType) {}
        DiskTypeEnum disc_type;
    };

    // Concrete FSM implementation 
    struct player_ : public msm::front::state_machine_def<player_>
    {
        // The list of FSM states
        struct Empty : public msm::front::state<> 
        {
            typedef mpl::vector<play> deferred_events;
            // every (optional) entry/exit methods get the event packed as boost::any. Not useful very often.
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "entering: Empty" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "leaving: Empty" << std::endl;}
        };
        struct Open : public msm::front::state<> 
        { 
            typedef mpl::vector1<CDLoaded>  flag_list;
            typedef mpl::vector<play>       deferred_events;
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "entering: Open" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "leaving: Open" << std::endl;}
        };
        // a state needing a pointer to the containing state machine
        // and using for this the non-default policy
        // if policy used, set_sm_ptr is needed
        struct Stopped : public msm::front::state<default_base_state,msm::front::sm_ptr> 
        { 
            // when stopped, the CD is loaded
            typedef mpl::vector1<CDLoaded>  flag_list;
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "entering: Stopped" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "leaving: Stopped" << std::endl;}
            void set_sm_ptr(player_* pl){m_player=pl;}
            player_* m_player;
        };
        // the player state machine contains a state which is himself a state machine
        // it demonstrates Shallow History: if the state gets activated with end_pause
        // then it will remember the last active state and reactivate it
        // also possible: AlwaysHistory, the last active state will always be reactivated
        // or NoHistory, always restart from the initial state
        struct Playing_ : public msm::front::state_machine_def<Playing_>
        {
            // when playing, the CD is loaded and we are in either pause or playing (duh)
            typedef mpl::vector<CDLoaded>       flag_list;
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "entering: Playing" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "leaving: Playing" << std::endl;}

            // The list of FSM states
            struct Song1 : public msm::front::state<>
            {
                typedef mpl::vector1<FirstSongPlaying>      flag_list;
                template <class Event,class FSM>
                void on_entry(Event const&,FSM& ) {std::cout << "starting: First song" << std::endl;}
                template <class Event,class FSM>
                void on_exit(Event const&,FSM& ) {std::cout << "finishing: First Song" << std::endl;}
            };
            struct Song2 : public msm::front::state<>
            { 
                template <class Event,class FSM>
                void on_entry(Event const&,FSM& ) {std::cout << "starting: Second song" << std::endl;}
                template <class Event,class FSM>
                void on_exit(Event const&,FSM& ) {std::cout << "finishing: Second Song" << std::endl;}
            };
            struct Song3 : public msm::front::state<>
            { 
                template <class Event,class FSM>
                void on_entry(Event const&,FSM& ) {std::cout << "starting: Third song" << std::endl;}
                template <class Event,class FSM>
                void on_exit(Event const&,FSM& ) {std::cout << "finishing: Third Song" << std::endl;}
            };
            struct CDFinished : public msm::front::exit_pseudo_state<AllSongsPlayed> 
            {
                template <class Event,class FSM>
                void on_entry(Event const&,FSM& ) {std::cout << "entering: Playing::CDFinished" << std::endl;}
                template <class Event,class FSM>
                void on_exit(Event const&,FSM& ) {std::cout << "leaving: Playing::CDFinished" << std::endl;}
            };
            // the initial state. Must be defined
            typedef Song1 initial_state;
            // transition actions
            void start_next_song(NextSong const&)       { std::cout << "Playing::start_next_song\n"; }
            void start_prev_song(PreviousSong const&)       { std::cout << "Playing::start_prev_song\n"; }
            void all_songs_played(NextSong const&)       { std::cout << "Playing::all_songs_played\n"; }
           // guard conditions

            typedef Playing_ pl; // makes transition table cleaner
            // Transition table for Playing
            struct transition_table : mpl::vector<
                //    Start     Event           Next         Action               Guard
                //  +---------+---------------+------------+---------------------+----------------------+
                a_row < Song1   , NextSong    , Song2      , &pl::start_next_song                      >,
                a_row < Song2   , PreviousSong, Song1      , &pl::start_prev_song                      >,
                a_row < Song2   , NextSong    , Song3      , &pl::start_next_song                      >,
                a_row < Song3   , PreviousSong, Song2      , &pl::start_prev_song                      >,
                a_row < Song3   , NextSong    , CDFinished , &pl::all_songs_played                     >
                //  +---------+---------------+---------+---------------------+----------------------+
            > {};
            // Replaces the default no-transition response.
            template <class FSM,class Event>
            void no_transition(Event const& e, FSM&,int state)
            {
                std::cout << "no transition from state " << state
                    << " on event " << typeid(e).name() << std::endl;
            }
        };
        typedef msm::back::state_machine<Playing_,msm::back::ShallowHistory<mpl::vector<end_pause> > > Playing;

        // the player state machine contains a state which is himself a state machine (2 of them, Playing and Paused)
        struct Paused : public msm::front::state<>
        {
            typedef mpl::vector<CDLoaded>       flag_list;
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "entering: Paused" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "leaving: Paused" << std::endl;}
        };

        struct AllOk : public msm::front::state<>
        {
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "starting: AllOk" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "finishing: AllOk" << std::endl;}
        };
        struct ErrorMode : //public msm::front::terminate_state<>
            public msm::front::interrupt_state<end_error>
        {
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "starting: ErrorMode" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "finishing: ErrorMode" << std::endl;}
        };

        // the initial state of the player SM. Must be defined
        typedef mpl::vector<Empty,AllOk> initial_state;

        // transition actions
        void start_playback(play const&)       { std::cout << "player::start_playback\n"; }
        void open_drawer(open_close const&)    { std::cout << "player::open_drawer\n"; }
        void close_drawer(open_close const&)   { std::cout << "player::close_drawer\n"; }
        void store_cd_info(cd_detected const&) 
        { 
            std::cout << "player::store_cd_info\n"; 
            // generate another event to test the queue
            //cd.m_player.process_event(play());
        }
        void stop_playback(stop const&)        { std::cout << "player::stop_playback\n"; }
        void end_playback (AllSongsPlayed const&)   { std::cout << "player::end_playback\n"; }
        void pause_playback(pause const&)      { std::cout << "player::pause_playback\n"; }
        void resume_playback(end_pause const&)      { std::cout << "player::resume_playback\n"; }
        void stop_and_open(open_close const&)  { std::cout << "player::stop_and_open\n"; }
        void stopped_again(stop const&){std::cout << "player::stopped_again\n";}
        void report_error(error_found const&) {std::cout << "player::report_error\n";}
        void report_end_error(end_error const&) {std::cout << "player::report_end_error\n";}
        // guard conditions
        bool good_disk_format(cd_detected const& evt)
        {
            // to test a guard condition, let's say we understand only CDs, not DVD
            if (evt.disc_type != DISK_CD)
            {
                std::cout << "wrong disk, sorry" << std::endl;
                return false;
            }
            return true;
        }

        typedef player_ p; // makes transition table cleaner

        // Transition table for player
        struct transition_table : mpl::vector<
          //    Start         Event           Next      Action              Guard
          //  +-------------+---------------+---------+---------------------+----------------------+
         a_row < Stopped    , play          , Playing , &p::start_playback                        >,
         a_row < Stopped    , open_close    , Open    , &p::open_drawer                           >,
         a_row < Stopped    , stop          , Stopped , &p::stopped_again                         >,
          //  +-------------+---------------+---------+---------------------+----------------------+
         a_row < Open       , open_close    , Empty   , &p::close_drawer                          >,
          //  +-------------+---------------+---------+---------------------+----------------------+
         a_row < Empty      , open_close    , Open    , &p::open_drawer                           >,
           row < Empty      , cd_detected   , Stopped , &p::store_cd_info   ,&p::good_disk_format >,
          //  +-------------+---------------+---------+---------------------+----------------------+
         a_row < Playing    , stop          , Stopped , &p::stop_playback                         >,
         a_row < Playing    , pause         , Paused  , &p::pause_playback                        >,
         a_row < Playing    , open_close    , Open    , &p::stop_and_open                         >,
         a_row < Playing::exit_pt<
                 Playing_::CDFinished> , AllSongsPlayed, Stopped , &p::end_playback                          >,
          //  +-------------+---------------+---------+---------------------+----------------------+
         a_row < Paused     , end_pause     , Playing , &p::resume_playback                       >,
         a_row < Paused     , stop          , Stopped , &p::stop_playback                         >,
         a_row < Paused     , open_close    , Open    , &p::stop_and_open                         >,
          //  +-------------+---------------+---------+---------------------+----------------------+
         a_row < AllOk      , error_found   ,ErrorMode, &p::report_error                          >,
         a_row < ErrorMode  ,end_error      ,AllOk    , &p::report_end_error                      >
          //  +-------------+---------------+---------+---------------------+----------------------+
        > {};


        // Replaces the default no-transition response.
        template <class FSM,class Event>
        void no_transition(Event const& e, FSM&,int state)
        {
            std::cout << "no transition from state " << state
                << " on event " << typeid(e).name() << std::endl;
        }
    };
    typedef msm::back::state_machine<player_> player;

    //
    // Testing utilities.
    //

    void pstate(player const& p)
    {
        typedef player::stt Stt;
        typedef msm::back::generate_state_set<Stt>::type all_states;
        static char const* state_names[mpl::size<all_states>::value];
        // fill the names of the states defined in the state machine
        mpl::for_each<all_states,boost::msm::wrap<mpl::placeholders::_1> >
            (msm::back::fill_state_names<Stt>(state_names));

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
        std::cout << "CDLoaded active:" << std::boolalpha << p.is_flag_active<CDLoaded>() << std::endl; //=> false (no CD yet)

        // test deferred event
        // deferred in Empty and Open, will be handled only after event cd_detected
        std::cout << "play is not handled in the current state but is marked as delayed" << std::endl;
        p.process_event(play()); pstate(p);
        std::cout << "cd_detected will cause play to be handled also" << std::endl;
        // will be rejected, wrong disk type
        p.process_event(cd_detected(DISK_DVD)); pstate(p);
        // will be accepted, wrong disk type
        p.process_event(cd_detected(DISK_CD)); pstate(p);

        std::cout << "FirstSong active:" << std::boolalpha << p.is_flag_active<FirstSongPlaying>() << std::endl; //=> true
        p.process_event(NextSong());pstate(p);
        // We are now in second song, Flag inactive
        std::cout << "FirstSong active:" << std::boolalpha << p.is_flag_active<FirstSongPlaying>() << std::endl;//=> false
        p.process_event(NextSong());pstate(p);
        // 2nd song active
        p.process_event(PreviousSong());pstate(p);
        // Pause
        p.process_event(pause()); pstate(p);
        // go back to Playing
        // but end_pause is an event activating the History
        // => keep the last active State (SecondSong)
        p.process_event(end_pause());  pstate(p);
        // force an exit by listening all the songs
        p.process_event(NextSong());
        p.process_event(NextSong());pstate(p);

        std::cout << "CDLoaded active:" << std::boolalpha << p.is_flag_active<CDLoaded>() << std::endl;//=> true
        std::cout << "FirstSong active:" << std::boolalpha << p.is_flag_active<FirstSongPlaying>() << std::endl;//=> false

        // go back to Playing
        // but play is not leading to Shallow History => do not remember the last active State (SecondSong)
        // and activate again FirstSong and LightOn
        p.process_event(play());  pstate(p);
        p.process_event(error_found());  pstate(p);

        // try generating more events
        std::cout << "Trying to generate another event" << std::endl; // will not work, fsm is terminated or interrupted
        p.process_event(NextSong());pstate(p);

        std::cout << "Trying to end the error" << std::endl; // will work only if ErrorMode is interrupt state
        p.process_event(end_error());pstate(p);
        std::cout << "Trying to generate another event" << std::endl; // will work only if ErrorMode is interrupt state
        p.process_event(NextSong());pstate(p);

        // the states and events of the higher level FSM (player)
        typedef player::stt Stt;
        typedef msm::back::generate_state_set<Stt>::type simple_states;

        std::cout << "the state list:" << std::endl;
        mpl::for_each<simple_states,boost::msm::wrap<mpl::placeholders::_1> >(msm::back::display_type ());

        std::cout << "the event list:" << std::endl;
        typedef msm::back::generate_event_set<Stt>::type event_list;
        mpl::for_each<event_list,boost::msm::wrap<mpl::placeholders::_1> >(msm::back::display_type ());
        std::cout << std::endl;

        // the states and events recursively searched
        typedef msm::back::recursive_get_transition_table<player>::type recursive_stt;

        std::cout << "the state list (including sub-SMs):" << std::endl;

        typedef msm::back::generate_state_set<recursive_stt>::type all_states;
        mpl::for_each<all_states,boost::msm::wrap<mpl::placeholders::_1> >(msm::back::display_type ());

        std::cout << "the event list (including sub-SMs):" << std::endl;
        typedef msm::back::generate_event_set<recursive_stt>::type all_events;
        mpl::for_each<all_events,boost::msm::wrap<mpl::placeholders::_1> >(msm::back::display_type ());

    }
}

int main()
{
    test();
    return 0;
}


