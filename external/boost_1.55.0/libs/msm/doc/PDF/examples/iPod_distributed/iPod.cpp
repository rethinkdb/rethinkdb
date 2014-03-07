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
#include <set>
#include <string>
#include <iostream>
#define FUSION_MAX_VECTOR_SIZE 20

#include "boost/mpl/vector/vector50.hpp"
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>

#include "Events.hpp"
#include "PlayingMode.hpp"
#include "MenuMode.hpp"

using namespace std;
namespace msm = boost::msm;

namespace  // Concrete FSM implementation
{
    struct iPod_;
    typedef msm::back::state_machine<iPod_, 
        ::boost::msm::back::favor_compile_time> iPod;

    // Concrete FSM implementation 
    struct iPod_ : public msm::front::state_machine_def<iPod_>
    {
        // The list of FSM states
        struct NotHolding : public msm::front::state<>
        {
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "starting: NotHolding" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "finishing: NotHolding" << std::endl;}
        };
        struct Holding : public msm::front::interrupt_state<NoHold>
        {
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "starting: Holding" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "finishing: Holding" << std::endl;}
        };
        struct NotPlaying : public msm::front::state<>
        {
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "starting: NotPlaying" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "finishing: NotPlaying" << std::endl;}
        };
        struct NoMenuMode : public msm::front::state<>
        {
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "starting: NoMenuMode" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "finishing: NoMenuMode" << std::endl;}
        };
        struct NoOnOffButton : public msm::front::state<>
        {
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "starting: NoOnOffButton" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "finishing: NoOnOffButton" << std::endl;}
        };
        struct OffDown : public msm::front::state<>
        {
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "starting: OffDown, start timer" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "finishing: OffDown, end timer" << std::endl;}
        };
        struct PlayerOff : public msm::front::state<>
        {
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "starting: PlayerOff" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "finishing: PlayerOff" << std::endl;}
        };
        struct CheckMiddleButton : public msm::front::state<>
        {
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "starting: CheckMiddleButton" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "finishing: CheckMiddleButton" << std::endl;}
        };      

        // the initial state of the player SM. Must be defined
        typedef mpl::vector5<NotHolding,NotPlaying,NoMenuMode,NoOnOffButton,CheckMiddleButton> 
                                    initial_state;
        // transition actions
        void send_ActivateMenu(EndPlay const&)
        {
            std::cout << "leaving Playing" << std::endl;
            // we need to activate the menu and simulate its button
            (static_cast<iPod*>(this))->process_event(MenuButton());
        }
        void send_StartSong(CloseMenu const&)
        {
            // we suppose the 5th song was selected
           (static_cast<iPod*>(this))->process_event(StartSong(5));
        }
        void send_PlayPause(SouthReleased const&)
        {
            // action using the message queue to generate another event
            (static_cast<iPod*>(this))->process_event(PlayPause());
        }
        void send_Off(OnOffTimer const&)
        {
            std::cout << "turning player off" << std::endl;
            (static_cast<iPod*>(this))->process_event(Off());
        }
        void send_PlayingMiddleButton(MiddleButton const&)
        {
            (static_cast<iPod*>(this))->process_event(PlayingMiddleButton());
        }
        void send_MenuMiddleButton(MiddleButton const&)
        {
            (static_cast<iPod*>(this))->process_event(MenuMiddleButton());
        }
        // guard conditions
        bool is_menu(MiddleButton const&)
        {
            return (static_cast<iPod*>(this))->is_flag_active<MenuActive>();
        }
        bool is_no_menu(MiddleButton const& evt)
        {
            return !is_menu(evt);
        }
        template <class EVENT>
        void switch_on(EVENT const&)
        {
            std::cout << "turning player on" << std::endl;
        }
        typedef iPod_ fsm; // makes transition table cleaner

        // Transition table for player
        struct transition_table : mpl::vector<
        //     Start               Event           Next                Action                           Guard
        //    +-------------------+---------------+-------------------+--------------------------------+----------------------+
        _row < NotHolding         , Hold          , Holding                                                                   >,
        _row < Holding            , NoHold        , NotHolding                                                                >,
        //    +-------------------+---------------+-------------------+--------------------------------+----------------------+
        _row < NotPlaying         , PlayPause     , PlayingMode                                                               >,
       a_row < PlayingMode::exit_pt<PlayingMode_::
                PlayingExit>       , EndPlay       , NotPlaying        , &fsm::send_ActivateMenu                               >,
        //    +-------------------+---------------+-------------------+--------------------------------+----------------------+
        _row < NoMenuMode         , MenuButton    , MenuMode                                                                  >,
       a_row < MenuMode::exit_pt<MenuMode_::
                MenuExit>          , CloseMenu     , NoMenuMode        , &fsm::send_StartSong                                  >,
        //    +-------------------+---------------+-------------------+--------------------------------+----------------------+
        _row < NoOnOffButton      , SouthPressed  , OffDown                                                                   >,
       a_row < OffDown            , SouthReleased , NoOnOffButton     , &fsm::send_PlayPause                                  >,
       a_row < OffDown            , OnOffTimer    , PlayerOff         , &fsm::send_Off                                        >,
       a_row < PlayerOff          , SouthPressed  , NoOnOffButton     , &fsm::switch_on                                       >,
       a_row < PlayerOff          , NoHold        , NoOnOffButton     , &fsm::switch_on                                       >,
       //     +-------------------+---------------+--------------------+--------------------------------+----------------------+
         row < CheckMiddleButton  , MiddleButton  , CheckMiddleButton , &fsm::send_PlayingMiddleButton  , &fsm::is_menu       >,
         row < CheckMiddleButton  , MiddleButton  , CheckMiddleButton , &fsm::send_MenuMiddleButton     , &fsm::is_no_menu    >
        //    +-------------------+---------------+--------------------+--------------------------------+----------------------+
        > {};
       
        // Replaces the default no-transition response.
        template <class FSM,class Event>
        void no_transition(Event const& e, FSM&,int state)
        {
            std::cout << "no transition from state " << state
                << " on event " << typeid(e).name() << std::endl;
        }
    };
   
    void test()
    {
        iPod sm;
        sm.start();
        // we first press Hold
        std::cout << "pressing hold" << std::endl;
        sm.process_event(Hold());
        // pressing a button is now ignored
        std::cout << "pressing a button" << std::endl;
        sm.process_event(SouthPressed());
        // or even one contained in a submachine
        sm.process_event(EastPressed());
        // no more holding
        std::cout << "no more holding, end interrupt event sent" << std::endl;
        sm.process_event(NoHold());
        std::cout << "pressing South button a short time" << std::endl;
        sm.process_event(SouthPressed());
        // we suppose a short pressing leading to playing a song
        sm.process_event(SouthReleased());
        // we move to the next song
        std::cout << "we move to the next song" << std::endl;
        sm.process_event(NextSong());
        // then back to no song => exit from playing, menu active
        std::cout << "we press twice the West button (simulated)=> end of playing" << std::endl;
        sm.process_event(PreviousSong());
        sm.process_event(PreviousSong());
        // even in menu mode, pressing play will start playing the first song
        std::cout << "pressing play/pause" << std::endl;
        sm.process_event(SouthPressed());
        sm.process_event(SouthReleased());
        // of course pausing must be possible
        std::cout << "pressing play/pause" << std::endl;
        sm.process_event(SouthPressed());
        sm.process_event(SouthReleased());
        std::cout << "pressing play/pause" << std::endl;
        sm.process_event(SouthPressed());
        sm.process_event(SouthReleased());
        // while playing, you can fast forward
        std::cout << "pressing East button a long time" << std::endl;
        sm.process_event(EastPressed());
        // let's suppose the timer just fired
        sm.process_event(ForwardTimer());
        sm.process_event(ForwardTimer());
        // end of fast forwarding
        std::cout << "releasing East button" << std::endl;
        sm.process_event(EastReleased());
        // we now press the middle button to set playing at a given position
        std::cout << "pressing Middle button, fast forwarding disabled" << std::endl;
        sm.process_event(MiddleButton());
        std::cout <<"pressing East button to fast forward" << std::endl;
        sm.process_event(EastPressed());
        // we switch off and on
        std::cout <<"switch off player" << std::endl;
        sm.process_event(SouthPressed());
        sm.process_event(OnOffTimer());
        std::cout <<"switch on player" << std::endl;    
        sm.process_event(SouthPressed());
    }
}

int main()
{
    test();
    return 0;
}
