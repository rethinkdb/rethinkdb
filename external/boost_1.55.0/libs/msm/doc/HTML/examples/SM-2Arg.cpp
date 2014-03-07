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
#include <string>
#include "boost/mpl/vector/vector30.hpp"

#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/back/tools.hpp>

using namespace std;
namespace msm = boost::msm;

namespace  // Concrete FSM implementation
{
    // events
    struct play {};
    struct end_pause {};
    struct stop {};
    struct pause {};
    struct open_close {};
    struct NextSong {};
    struct PreviousSong {};
    struct ThreeSec {};
    struct TenSec {};
    struct go_sleep {};
    struct error_found {};
    struct end_error {};

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
    // an easy visitor
    struct SomeVisitor
    {
        template <class T>
        void visit_state(T* astate,int i)
        {
            std::cout << "visiting state:" << typeid(*astate).name() 
                << " with data:" << i << std::endl;
        }
    };
    // overwrite of the base state (not default)
    struct my_visitable_state
    {
        // signature of the accept function
        typedef msm::back::args<void,SomeVisitor&,int> accept_sig;

        // we also want polymorphic states
        virtual ~my_visitable_state() {}
        // default implementation for states who do not need to be visited
        void accept(SomeVisitor&,int) const {}
    };

    // Concrete FSM implementation 
    struct player_ : public msm::front::state_machine_def<player_,my_visitable_state>
    {
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& ) {std::cout << "starting: player" << std::endl;}
        // The list of FSM states
        struct Empty : public msm::front::state<my_visitable_state> 
        {
            typedef mpl::vector<play> deferred_events;
            // every (optional) entry/exit methods get the event packed as boost::any. Not useful very often.
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "entering: Empty" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "leaving: Empty" << std::endl;}
            void accept(SomeVisitor& vis,int i) const
            {
                vis.visit_state(this,i);
            }
        };
        struct Open : public msm::front::state<my_visitable_state> 
        {
            typedef mpl::vector1<CDLoaded>      flag_list;
            typedef mpl::vector<play> deferred_events;
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "entering: Open" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "leaving: Open" << std::endl;}
            void accept(SomeVisitor& vis,int i) const
            {
                vis.visit_state(this,i);
            }
        };
        // a state needing a pointer to the containing state machine
        // and using for this the non-default policy
        // if policy used, set_sm_ptr is needed
        struct Stopped : public msm::front::state<my_visitable_state> 
        {     
            // when stopped, the CD is loaded
            typedef mpl::vector1<CDLoaded>      flag_list;
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "entering: Stopped" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "leaving: Stopped" << std::endl;}
        };
        // the player state machine contains a state which is himself a state machine
        // it demonstrates Shallow History: if the state gets activated with end_pause
        // then it will remember the last active state and reactivate it
        // also possible: AlwaysHistory, the last active state will always be reactivated
        // or NoHistory, always restart from the initial state
        struct Playing_ : public msm::front::state_machine_def<Playing_,my_visitable_state >
        {
            // when playing, the CD is loaded and we are in either pause or playing (duh)
            typedef mpl::vector2<PlayingPaused,CDLoaded>        flag_list;
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "entering: Playing" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "leaving: Playing" << std::endl;}
            void accept(SomeVisitor& vis,int i) const
            {
                // note that visiting will recursively visit sub-states
                vis.visit_state(this,i);
            }
            // The list of FSM states
            // the Playing state machine contains a state which is himself a state machine
            // so we have a SM containing a SM containing a SM
            struct Song1_ : public msm::front::state_machine_def<Song1_,my_visitable_state>
            {
                typedef mpl::vector1<FirstSongPlaying>      flag_list;
                template <class Event,class FSM>
                void on_entry(Event const&,FSM& ) {std::cout << "starting: First song" << std::endl;}
                template <class Event,class FSM>
                void on_exit(Event const&,FSM& ) {std::cout << "finishing: First Song" << std::endl;}
                void accept(SomeVisitor& vis,int i) const
                {
                    vis.visit_state(this,i);
                }
                struct LightOn : public msm::front::state<my_visitable_state>
                { 
                    template <class Event,class FSM>
                    void on_entry(Event const&,FSM& ) {std::cout << "starting: LightOn" << std::endl;}
                    template <class Event,class FSM>
                    void on_exit(Event const&,FSM& ) {std::cout << "finishing: LightOn" << std::endl;}
                };
                struct LightOff : public msm::front::state<my_visitable_state>
                { 
                    template <class Event,class FSM>
                    void on_entry(Event const&,FSM& ) {std::cout << "starting: LightOff" << std::endl;}
                    template <class Event,class FSM>
                    void on_exit(Event const&,FSM& ) {std::cout << "finishing: LightOff" << std::endl;}
                };
                // the initial state. Must be defined
                typedef LightOn initial_state;
                // transition actions
                void turn_light_off(ThreeSec const&)       { std::cout << "3s off::turn light off\n"; }
                // guard conditions

                typedef Song1_ s; // makes transition table cleaner
                // Transition table for Song1
                struct transition_table : mpl::vector1<
                    //    Start     Event         Next      Action               Guard
                    //  +---------+-------------+---------+---------------------+----------------------+
                  a_row < LightOn , ThreeSec    , LightOff, &s::turn_light_off                        >
                    //  +---------+-------------+---------+---------------------+----------------------+
                > {};
                // Replaces the default no-transition response.
                template <class FSM,class Event>
                void no_transition(Event const& e, FSM&,int state)
                {
                    std::cout << "no transition from state " << state
                        << " on event " << typeid(e).name() << std::endl;
                }
            };
            typedef msm::back::state_machine<Song1_> Song1;

            struct Song2 : public msm::front::state<my_visitable_state>
            {
                template <class Event,class FSM>
                void on_entry(Event const&,FSM& ) {std::cout << "starting: Second song" << std::endl;}
                template <class Event,class FSM>
                void on_exit(Event const&,FSM& ) {std::cout << "finishing: Second Song" << std::endl;}
            };
            struct Song3 : public msm::front::state<my_visitable_state>
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
                //    Start     Event         Next      Action               Guard
                //  +---------+-------------+---------+---------------------+----------------------+
                a_row < Song1   , NextSong    , Song2   , &pl::start_next_song                      >,
                a_row < Song2   , PreviousSong, Song1   , &pl::start_prev_song                      >,
                a_row < Song2   , NextSong    , Song3   , &pl::start_next_song                      >,
                a_row < Song3   , PreviousSong, Song2   , &pl::start_prev_song                      >
                //  +---------+-------------+---------+---------------------+----------------------+
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
        struct Paused_ : public msm::front::state_machine_def<Paused_,my_visitable_state>
        {
            typedef mpl::vector2<PlayingPaused,CDLoaded>    flag_list;
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "entering: Paused" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "leaving: Paused" << std::endl;}

            // The list of FSM states
            struct StartBlinking : public msm::front::state<my_visitable_state>
            { 
                template <class Event,class FSM>
                void on_entry(Event const&,FSM& ) {std::cout << "starting: StartBlinking" << std::endl;}
                template <class Event,class FSM>
                void on_exit(Event const&,FSM& ) {std::cout << "finishing: StartBlinking" << std::endl;}
            };
            struct StopBlinking : public msm::front::state<my_visitable_state>
            { 
                template <class Event,class FSM>
                void on_entry(Event const&,FSM& ) {std::cout << "starting: StopBlinking" << std::endl;}
                template <class Event,class FSM>
                void on_exit(Event const&,FSM& ) {std::cout << "finishing: StopBlinking" << std::endl;}
            };
            // the initial state. Must be defined
            typedef StartBlinking initial_state;
            // transition actions
            void start_blinking(TenSec const&)       { std::cout << "Paused::start_blinking\n"; }
            void stop_blinking(TenSec const&)       { std::cout << "Paused::stop_blinking\n"; }
            // guard conditions

            typedef Paused_ pa; // makes transition table cleaner
            // Transition table
            struct transition_table : mpl::vector2<
                //    Start          Event         Next           Action                Guard
                //  +---------------+-------------+--------------+---------------------+----------------------+
                a_row < StartBlinking , TenSec      , StopBlinking  , &pa::stop_blinking                        >,
                a_row < StopBlinking  , TenSec      , StartBlinking , &pa::start_blinking                       >
                //  +---------------+-------------+---------------+--------------------+----------------------+
            > {};
            // Replaces the default no-transition response.
            template <class FSM,class Event>
            void no_transition(Event const& e, FSM&,int state)
            {
                std::cout << "no transition from state " << state
                    << " on event " << typeid(e).name() << std::endl;
            }
        };
        typedef msm::back::state_machine<Paused_> Paused;

        struct SleepMode : public msm::front::state<my_visitable_state> 
        {
        }; // dummy state just to test the automatic id generation

        struct AllOk : public msm::front::state<my_visitable_state>
        {
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "starting: AllOk" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "finishing: AllOk" << std::endl;}
        };
        struct ErrorMode : //public terminate_state<>
            public msm::front::interrupt_state<end_error,my_visitable_state>
        {
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "starting: ErrorMode" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "finishing: ErrorMode" << std::endl;}
        };

        // the initial state of the player SM. Must be defined
        typedef mpl::vector<Empty,AllOk> initial_state;
        //typedef Empty initial_state; // this is to have only one active state
 
        // transition actions
        void start_playback(play const&)       { std::cout << "player::start_playback\n"; }
        void open_drawer(open_close const&)    { std::cout << "player::open_drawer\n"; }
        void close_drawer(open_close const&)   { std::cout << "player::close_drawer\n"; }
        void store_cd_info(cd_detected const&) 
        { 
            std::cout << "player::store_cd_info\n"; 
            // generate another event to test the queue
            //process_event(play());
        }
        void stop_playback(stop const&)        { std::cout << "player::stop_playback\n"; }
        void pause_playback(pause const&)      { std::cout << "player::pause_playback\n"; }
        void resume_playback(end_pause const&)      { std::cout << "player::resume_playback\n"; }
        void stop_and_open(open_close const&)  { std::cout << "player::stop_and_open\n"; }
        void stopped_again(stop const&){std::cout << "player::stopped_again\n";}
        void start_sleep(go_sleep const&)  {  }
        void report_error(error_found const&) {std::cout << "player::report_error\n";}
        void report_end_error(end_error const&) {std::cout << "player::report_end_error\n";}
        // guard conditions


        typedef player_ p; // makes transition table cleaner

        // Transition table for player
        struct transition_table : mpl::vector<
            //    Start     Event         Next      Action               Guard
            //  +---------+-------------+---------+---------------------+----------------------+
          a_row < Stopped , play        , Playing , &p::start_playback                        >,
          a_row < Stopped , open_close  , Open    , &p::open_drawer                           >,
          a_row < Stopped , stop        , Stopped , &p::stopped_again                         >,
            //  +---------+-------------+---------+---------------------+----------------------+
          a_row < Open    , open_close  , Empty   , &p::close_drawer                          >,
            //  +---------+-------------+---------+---------------------+----------------------+
          a_row < Empty   , open_close  , Open    , &p::open_drawer                           >,
          a_row < Empty   , cd_detected , Stopped , &p::store_cd_info                         >,
            //  +---------+-------------+---------+---------------------+----------------------+
          a_row < Playing , stop        , Stopped , &p::stop_playback                         >,
          a_row < Playing , pause       , Paused  , &p::pause_playback                        >,
          a_row < Playing , open_close  , Open    , &p::stop_and_open                         >,
            //  +---------+-------------+---------+---------------------+----------------------+
          a_row < Paused  , end_pause   , Playing , &p::resume_playback                       >,
          a_row < Paused  , stop        , Stopped , &p::stop_playback                         >,
          a_row < Paused  , open_close  , Open    , &p::stop_and_open                         >,
          a_row < Paused  , go_sleep    ,SleepMode, &p::start_sleep                           >,
            //  +---------+-------------+---------+---------------------+----------------------+
          a_row < AllOk   , error_found ,ErrorMode, &p::report_error                          >,
          a_row <ErrorMode,end_error    ,AllOk    , &p::report_end_error                      >
            //  +---------+-------------+---------+---------------------+----------------------+
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
    typedef msm::back::state_machine<player_> player;

    //
    // Testing utilities.
    //

    void pstate(player const& p)
    {
        static char const* const state_names[] = { "Stopped", "Open", "Empty", "Playing", "Paused","AllOk","ErrorMode","SleepMode" };
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
        p.process_event(play());

        // go to Open, call on_exit on Empty, then action, then on_entry on Open
        p.process_event(open_close()); pstate(p);
        // visiting Paused and AllOk, but only Paused cares
        SomeVisitor vis;
        p.visit_current_states(boost::ref(vis),1);
        p.process_event(open_close()); pstate(p);
        // visiting Empty and AllOk, but only Empty cares
        p.visit_current_states(boost::ref(vis),2);


        p.process_event(cd_detected("louie, louie"));
        // no need to call play() as the previous event does it in its action method
        //p.process_event(play());  
        // at this point, Play is active, along FirstSong and LightOn
        pstate(p);
        // visiting Playing+Song1 and AllOk, but only Playing+Song1 care
        p.visit_current_states(boost::ref(vis),3);

        std::cout << "PlayingPaused active:" << std::boolalpha << p.is_flag_active<PlayingPaused>() << std::endl;//=> true
        // call on_exit on LightOn,FirstSong,Play like stated in the UML spec.
        // and of course on_entry on Paused and StartBlinking
        p.process_event(pause()); pstate(p);
        std::cout << "PlayingPaused active:" << std::boolalpha << p.is_flag_active<PlayingPaused>() << std::endl;//=> true
        // forward events to Paused
        p.process_event(TenSec());
        p.process_event(TenSec());
        // go back to Playing
        p.process_event(end_pause());  pstate(p);
        std::cout << "FirstSong active:" << std::boolalpha << p.is_flag_active<FirstSongPlaying>() << std::endl; //=> true
        p.process_event(ThreeSec());  pstate(p);
        p.process_event(NextSong());pstate(p);
        // We are now in second song, Flag inactive
        std::cout << "FirstSong active:" << std::boolalpha << p.is_flag_active<FirstSongPlaying>() << std::endl;//=> false
        // visiting Playing+Song2 and AllOk, but only Playing cares
        p.visit_current_states(boost::ref(vis),4);

        p.process_event(NextSong());pstate(p);
        // 2nd song active
        p.process_event(PreviousSong());pstate(p);
        // Pause
        p.process_event(pause()); pstate(p);
        // go back to Playing
        // but end_pause is an event activating the History
        // => keep the last active State (SecondSong)
        p.process_event(end_pause());  pstate(p);
        // test of an event from a state to itself. According to UML spec, call again exit/entry from Stopped
        p.process_event(stop());  pstate(p);
        p.process_event(stop());  pstate(p);
        std::cout << "CDLoaded active:" << std::boolalpha << p.is_flag_active<CDLoaded>() << std::endl;//=> true
        std::cout << "PlayingPaused active:" << std::boolalpha << p.is_flag_active<PlayingPaused>() << std::endl;//=> false
        std::cout << "FirstSong active:" << std::boolalpha << p.is_flag_active<FirstSongPlaying>() << std::endl;//=> false
        std::cout << "CDLoaded active with AND:" << std::boolalpha << p.is_flag_active<CDLoaded,player::Flag_AND>() << std::endl;//=> false
        std::cout << "CDLoaded active with OR:" << std::boolalpha << p.is_flag_active<CDLoaded,player::Flag_OR>() << std::endl;//=> true

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

        std::cout << "Simulate error. Event play is not valid" << std::endl;
        p.process_event(play()); pstate(p);
    }
}

int main()
{
    test();
    return 0;
}


