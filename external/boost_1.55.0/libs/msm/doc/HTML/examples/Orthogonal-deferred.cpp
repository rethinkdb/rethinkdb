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
    struct end_error2 {};

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
        template <class Event,class FSM>
        void on_entry(Event const& ,FSM&) 
        {
            std::cout << "entering: Player" << std::endl;
        }
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) 
        {
            std::cout << "leaving: Player" << std::endl;
        }
        // The list of FSM states
        struct Empty : public msm::front::state<> 
        {
            // if the play event arrives in this state, defer it until a state handles it or
            // rejects it
            typedef mpl::vector<play> deferred_events;
            // every (optional) entry/exit methods get the event passed.
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "entering: Empty" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "leaving: Empty" << std::endl;}
        };
        struct Open : public msm::front::state<> 
        { 
            // if the play event arrives in this state, defer it until a state handles it or
            // rejects it
            typedef mpl::vector<play> deferred_events;
            typedef mpl::vector1<CDLoaded>      flag_list;
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "entering: Open" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "leaving: Open" << std::endl;}
        };

        struct Stopped : public msm::front::state<> 
        { 
            // when stopped, the CD is loaded
            typedef mpl::vector1<CDLoaded>      flag_list;
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "entering: Stopped" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "leaving: Stopped" << std::endl;}
        };

        // the player state machine contains a state which is himself a state machine
        // as you see, no need to declare it anywhere so Playing can be developed separately
        // by another team in another module. For simplicity I just declare it inside player
        struct Playing_ : public msm::front::state_machine_def<Playing_>
        {
            // when playing, the CD is loaded and we are in either pause or playing (duh)
            typedef mpl::vector2<PlayingPaused,CDLoaded>        flag_list;

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
            // the initial state. Must be defined
            typedef Song1 initial_state;
            // transition actions
            void start_next_song(NextSong const&)       { std::cout << "Playing::start_next_song\n"; }
            void start_prev_song(PreviousSong const&)       { std::cout << "Playing::start_prev_song\n"; }
            // guard conditions

            typedef Playing_ pl; // makes transition table cleaner
            // Transition table for Playing
            struct transition_table : mpl::vector4<
                //      Start     Event         Next      Action               Guard
                //    +---------+-------------+---------+---------------------+----------------------+
                a_row < Song1   , NextSong    , Song2   , &pl::start_next_song                       >,
                a_row < Song2   , PreviousSong, Song1   , &pl::start_prev_song                       >,
                a_row < Song2   , NextSong    , Song3   , &pl::start_next_song                       >,
                a_row < Song3   , PreviousSong, Song2   , &pl::start_prev_song                       >
                //    +---------+-------------+---------+---------------------+----------------------+
            > {};
            // Replaces the default no-transition response.
            template <class FSM,class Event>
            void no_transition(Event const& e, FSM&,int state)
            {
                std::cout << "no transition from state " << state
                    << " on event " << typeid(e).name() << std::endl;
            }
        };
        // back-end
        typedef msm::back::state_machine<Playing_> Playing;

        // state not defining any entry or exit
        struct Paused : public msm::front::state<>
        {
            typedef mpl::vector2<PlayingPaused,CDLoaded>        flag_list;
        };
        struct AllOk : public msm::front::state<>
        {
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "starting: AllOk" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "finishing: AllOk" << std::endl;}
        };
        // this state is also made terminal so that all the events are blocked
        struct ErrorMode :  //public msm::front::terminate_state<> // ErrorMode terminates the state machine
            public msm::front::interrupt_state<end_error/*mpl::vector<end_error,end_error2>*/ >   // ErroMode just interrupts. Will resume if
                                                            // the event end_error is generated
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
        void store_cd_info(cd_detected const& cd) {std::cout << "player::store_cd_info\n";}
        void stop_playback(stop const&)        { std::cout << "player::stop_playback\n"; }
        void pause_playback(pause const&)      { std::cout << "player::pause_playback\n"; }
        void resume_playback(end_pause const&)      { std::cout << "player::resume_playback\n"; }
        void stop_and_open(open_close const&)  { std::cout << "player::stop_and_open\n"; }
        void stopped_again(stop const&){std::cout << "player::stopped_again\n";}
        void report_error(error_found const&) {std::cout << "player::report_error\n";}
        void report_end_error(end_error const&) {std::cout << "player::report_end_error\n";}

        // guard conditions
 
        typedef player_ p; // makes transition table cleaner

        // Transition table for player
        struct transition_table : mpl::vector<
            //      Start     Event         Next      Action               Guard
            //    +---------+-------------+---------+---------------------+----------------------+
            a_row < Stopped , play        , Playing , &p::start_playback                         >,
            a_row < Stopped , open_close  , Open    , &p::open_drawer                            >,
            a_row < Stopped , stop        , Stopped , &p::stopped_again                          >,
            //    +---------+-------------+---------+---------------------+----------------------+
            a_row < Open    , open_close  , Empty   , &p::close_drawer                           >,
            //    +---------+-------------+---------+---------------------+----------------------+
            a_row < Empty   , open_close  , Open    , &p::open_drawer                            >,
            a_row < Empty   , cd_detected , Stopped , &p::store_cd_info                          >,
            //    +---------+-------------+---------+---------------------+----------------------+
            a_row < Playing , stop        , Stopped , &p::stop_playback                          >,
            a_row < Playing , pause       , Paused  , &p::pause_playback                         >,
            a_row < Playing , open_close  , Open    , &p::stop_and_open                          >,
            //    +---------+-------------+---------+---------------------+----------------------+
            a_row < Paused  , end_pause   , Playing , &p::resume_playback                        >,
            a_row < Paused  , stop        , Stopped , &p::stop_playback                          >,
            a_row < Paused  , open_close  , Open    , &p::stop_and_open                          >,
            //    +---------+-------------+---------+---------------------+----------------------+
            a_row < AllOk   , error_found ,ErrorMode, &p::report_error                           >,
            a_row <ErrorMode,end_error    ,AllOk    , &p::report_end_error                       >
            //    +---------+-------------+---------+---------------------+----------------------+
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
    typedef msm::back::state_machine<player_> player;

    //
    // Testing utilities.
    //
    static char const* const state_names[] = { "Stopped", "Open", "Empty", "Playing", "Paused","AllOk","ErrorMode" };

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
        // test deferred event
        // deferred in Empty and Open, will be handled only after event cd_detected
        p.process_event(play());

        // tests some flags
        std::cout << "CDLoaded active:" << std::boolalpha << p.is_flag_active<CDLoaded>() << std::endl; //=> false (no CD yet)
        // go to Open, call on_exit on Empty, then action, then on_entry on Open
        p.process_event(open_close()); pstate(p);
        p.process_event(open_close()); pstate(p);
        p.process_event(cd_detected("louie, louie"));

        // at this point, Play is active (was deferred)
        std::cout << "PlayingPaused active:" << std::boolalpha << p.is_flag_active<PlayingPaused>() << std::endl;//=> true
        std::cout << "FirstSong active:" << std::boolalpha << p.is_flag_active<FirstSongPlaying>() << std::endl;//=> true

        // make transition happen inside it. Player has no idea about this event but it's ok.
        p.process_event(NextSong());pstate(p); //2nd song active
        p.process_event(NextSong());pstate(p);//3rd song active
        p.process_event(PreviousSong());pstate(p);//2nd song active
        std::cout << "FirstSong active:" << std::boolalpha << p.is_flag_active<FirstSongPlaying>() << std::endl;//=> false

        std::cout << "PlayingPaused active:" << std::boolalpha << p.is_flag_active<PlayingPaused>() << std::endl;//=> true
        p.process_event(pause()); pstate(p);
        std::cout << "PlayingPaused active:" << std::boolalpha << p.is_flag_active<PlayingPaused>() << std::endl;//=> true
        // go back to Playing
        // as you see, it starts back from the original state
        p.process_event(end_pause());  pstate(p);
        p.process_event(pause()); pstate(p);
        p.process_event(stop());  pstate(p);
        std::cout << "PlayingPaused active:" << std::boolalpha << p.is_flag_active<PlayingPaused>() << std::endl;//=> false
        std::cout << "CDLoaded active:" << std::boolalpha << p.is_flag_active<CDLoaded>() << std::endl;//=> true
        // by default, the flags are OR'ed but you can also use AND. Then the flag must be present in 
        // all of the active states
        std::cout << "CDLoaded active with AND:" << std::boolalpha << p.is_flag_active<CDLoaded,player::Flag_AND>() << std::endl;//=> false

        // event leading to the same state
        p.process_event(stop());  pstate(p);

        // event leading to a terminal/interrupt state
        p.process_event(error_found());  pstate(p);
        // try generating more events
        std::cout << "Trying to generate another event" << std::endl; // will not work, fsm is terminated or interrupted
        p.process_event(play());pstate(p);
        std::cout << "Trying to end the error" << std::endl; // will work only if ErrorMode is interrupt state
        p.process_event(end_error());pstate(p);
        std::cout << "Trying to generate another event" << std::endl; // will work only if ErrorMode is interrupt state
        p.process_event(play());pstate(p);
        std::cout << "stop fsm" << std::endl;
        p.stop();

    }
}

int main()
{
    test();
    return 0;
}
