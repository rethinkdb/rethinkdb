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
    struct PreviousSong {};
    struct ThreeSec {};
    struct TenSec {};
    struct error_found {};
    struct end_error {};
    struct cd_detected {};

    // a simple visitor
    struct SomeVisitor
    {
        template <class T>
        void visit_state(T* astate,int i)
        {
            std::cout << "visiting state:" << typeid(*astate).name() 
                << " with data:" << i << std::endl;
        }
    };
    // base state for all states of ths fsm, to make them visitable
    struct my_visitable_state
    {
        // signature of the accept function
        typedef msm::back::args<void,SomeVisitor&,int> accept_sig;

        // we also want polymorphic states
        virtual ~my_visitable_state() {}
        // default implementation for states who do not need to be visited
        void accept(SomeVisitor&,int) const {}
        // or if you want all states to be visited, provide an implementation
        /*
        void accept(SomeVisitor& vis,int i) const
        {
            vis.visit_state(this,i);
        }
        */
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
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "entering: Empty" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "leaving: Empty" << std::endl;}
            // this state wants to be visited
            void accept(SomeVisitor& vis,int i) const
            {
                vis.visit_state(this,i);
            }
        };
        struct Open : public msm::front::state<my_visitable_state> 
        {	
            typedef mpl::vector<play> deferred_events;
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "entering: Open" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "leaving: Open" << std::endl;}
            // this state wants to be visited
            void accept(SomeVisitor& vis,int i) const
            {
                vis.visit_state(this,i);
            }
        };
        struct Stopped : public msm::front::state<my_visitable_state> 
        {	     
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "entering: Stopped" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "leaving: Stopped" << std::endl;}
            // this state wants to be visited
            void accept(SomeVisitor& vis,int i) const
            {
                // note that visiting will recursively visit sub-states
                vis.visit_state(this,i);
            }
        };

        struct Playing_ : public msm::front::state_machine_def<Playing_,my_visitable_state >
        {
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
                    void accept(SomeVisitor& vis,int i) const
                    {
                        // note that visiting will recursively visit sub-states
                        vis.visit_state(this,i);
                    }
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
                    //    Start     Event         Next      Action				Guard
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
                //    Start     Event         Next      Action				Guard
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

        struct Paused : public msm::front::state<my_visitable_state>
        {
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "entering: Paused" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "leaving: Paused" << std::endl;}    
        };

        struct AllOk : public msm::front::state<my_visitable_state>
        {
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "starting: AllOk" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "finishing: AllOk" << std::endl;}
        };
        struct ErrorMode : 
            public msm::front::interrupt_state<end_error,my_visitable_state>
        {
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "starting: ErrorMode" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "finishing: ErrorMode" << std::endl;}
            void accept(SomeVisitor& vis,int i) const
            {
                vis.visit_state(this,i);
            }
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
        }
        void stop_playback(stop const&)        { std::cout << "player::stop_playback\n"; }
        void pause_playback(pause const&)      { std::cout << "player::pause_playback\n"; }
        void resume_playback(end_pause const&)      { std::cout << "player::resume_playback\n"; }
        void stop_and_open(open_close const&)  { std::cout << "player::stop_and_open\n"; }
        void stopped_again(stop const&)	{std::cout << "player::stopped_again\n";}
        void report_error(error_found const&) {std::cout << "player::report_error\n";}
        void report_end_error(end_error const&) {std::cout << "player::report_end_error\n";}
        // guard conditions


        typedef player_ p; // makes transition table cleaner

        // Transition table for player
        struct transition_table : mpl::vector<
            //    Start     Event         Next      Action				Guard
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

        p.process_event(cd_detected());
        // no need to call play() as the previous event does it in its action method
        //p.process_event(play());  
        // at this point, Play is active, along FirstSong and LightOn
        pstate(p);
        // visiting Playing+Song1+LightOn and AllOk, but only Playing+Song1+LightOn care
        p.visit_current_states(boost::ref(vis),3);

        // Stop will be active
        p.process_event(stop());  pstate(p);

        // visiting when both regions have an active state who wants to be visited
        p.process_event(error_found());
        p.visit_current_states(boost::ref(vis),5);


    }
}

int main()
{
    test();
    return 0;
}


