// Copyright 2010 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef PLAYING_MODE_HPP
#define PLAYING_MODE_HPP

#include <iostream>
#include <boost/any.hpp>
#define FUSION_MAX_VECTOR_SIZE 20

#include "Events.hpp"
#include <boost/msm/back/favor_compile_time.hpp>
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/euml/euml.hpp>

using namespace std;
namespace msm = boost::msm;
namespace euml = boost::msm::front::euml;

struct PlayingMode_ : public msm::front::state_machine_def<PlayingMode_>
{
    //flags 
    struct NoFastFwd{};

    struct Playing : public msm::front::state<default_base_state,msm::front::sm_ptr>
    {
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& ) 
        {
            std::cout << "starting: PlayingMode::Playing" << std::endl;
            std::cout << "playing song:" << m_fsm->get_current_song() << std::endl;
        }
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) {std::cout << "finishing: PlayingMode::Playing" << std::endl;}
        void set_sm_ptr(PlayingMode_* pl)
        {
            m_fsm = pl;
        }
    private:
        PlayingMode_* m_fsm;
    };
    struct Paused : public msm::front::state<>
    {
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& ) {std::cout << "starting: PlayingMode::Paused" << std::endl;}
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) {std::cout << "finishing: PlayingMode::Paused" << std::endl;}
    };
    struct WaitingForNextPrev : public msm::front::state<>
    {
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& ) {std::cout << "starting: PlayingMode::WaitingForNextPrev" << std::endl;}
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) {std::cout << "finishing: PlayingMode::WaitingForNextPrev" << std::endl;}
    };
    struct WaitingForEnd : public msm::front::state<>
    {
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& ) {std::cout << "starting: PlayingMode::WaitingForEnd" << std::endl;}
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) {std::cout << "finishing: PlayingMode::WaitingForEnd" << std::endl;}
    };
    struct NoForward : public msm::front::state<>
    {
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& ) {std::cout << "starting: PlayingMode::NoForward" << std::endl;}
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) {std::cout << "finishing: PlayingMode::NoForward" << std::endl;}
    };
    struct ForwardPressed : public msm::front::state<>
    {
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& ) 
        {
            std::cout << "starting: PlayingMode::ForwardPressed," << "start timer" << std::endl;
        }
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) 
        {
            std::cout << "finishing: PlayingMode::ForwardPressed," << "stop timer" << std::endl;
        }
    };
    struct FastForward : public msm::front::state<>
    {
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& ) 
        {
            std::cout << "starting: PlayingMode::FastForward," << "start timer" << std::endl;
        }
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) 
        {
            std::cout << "finishing: PlayingMode::FastForward," << "stop timer" << std::endl;
        }
    };
    struct StdDisplay : public msm::front::state<>
    {
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& ) {std::cout << "starting: PlayingMode::StdDisplay" << std::endl;}
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) {std::cout << "finishing: PlayingMode::StdDisplay" << std::endl;}
    };
    struct SetPosition : public msm::front::state<>
    {
        typedef mpl::vector1<NoFastFwd>     flag_list;
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& ) {std::cout << "starting: PlayingMode::SetPosition" << std::endl;}
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) {std::cout << "finishing: PlayingMode::SetPosition" << std::endl;}
    };
    struct SetMark : public msm::front::state<>
    {
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& ) {std::cout << "starting: PlayingMode::SetMark" << std::endl;}
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) {std::cout << "finishing: PlayingMode::SetMark" << std::endl;}
    };
    struct PlayingExit : public msm::front::exit_pseudo_state<EndPlay>
    {
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& ) {std::cout << "starting: PlayingMode::PlayingExit" << std::endl;}
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) {std::cout << "finishing: PlayingMode::PlayingExit" << std::endl;}
    };
    // transition action methods
    struct inc_song_counter : euml::euml_action<inc_song_counter>
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            if (++fsm.m_SongIndex <= fsm.m_NumberOfSongs )
            {
                std::cout << "playing song:" << fsm.m_SongIndex << std::endl;
            }
            else
            {
                // last song => end playing, next play will start at the beginning
                fsm.m_SongIndex = 1;
                fsm.process_event(EndPlay());
            }
        }
    };
 
    void select_song(StartSong const& evt)
    {
        if ((evt.m_Selected>0) && (evt.m_Selected<=m_NumberOfSongs))
        {
            m_SongIndex = evt.m_Selected;
            std::cout << "selecting song:" << m_SongIndex << std::endl;
        }
        else
        {
            // play current song
            std::cout << "selecting song:" << m_SongIndex << std::endl;
        }
    }
    struct dec_song_counter : euml::euml_action<dec_song_counter>
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            if (--fsm.m_SongIndex >0 )
            {
                std::cout << "playing song:" << fsm.m_SongIndex << std::endl;
            }
            else
            {
                // before first song => end playing
                fsm.m_SongIndex = 1;
                fsm.process_event(EndPlay());
            }
        }
    };
    struct send_NextSong : euml::euml_action<send_NextSong>
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            fsm.process_event(NextSong());
        }
    };

    void do_fast_forward(ForwardTimer const&)
    {
        std::cout << "moving song forward..." << std::endl;
    }

    // transition guard methods
    struct fast_fwd_ok : euml::euml_action<fast_fwd_ok>
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        bool operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            // guard accepts only if fast forward is possible (No SetPosition mode) 
            return !fsm.is_flag_active<NoFastFwd>();
        }
    };
    // initial states / orthogonal zones
    typedef mpl::vector5<Playing,WaitingForNextPrev,WaitingForEnd,NoForward,StdDisplay> 
                            initial_state;
    typedef PlayingMode_ fsm; // makes transition table cleaner
    // Transition table for player
    struct transition_table : mpl::vector19<
            //    Start                 Event                Next                 Action                     Guard
            //   +--------------------+---------------------+--------------------+--------------------------+----------------------+
            _row < Playing            , PlayPause           , Paused                                                               >,
            _row < Playing            , Off                 , Paused                                                               >,
           a_row < Playing            , StartSong           , Playing            , &fsm::select_song                               >,
            _row < Paused             , PlayPause           , Playing                                                              >,
 msm::front::Row < Playing            , SongFinished        , Playing            , inc_song_counter         , msm::front::none     >,
           a_row < Paused             , StartSong           , Playing            , &fsm::select_song                               >,
            //   +--------------------+---------------------+--------------------+--------------------------+----------------------+
 msm::front::Row < WaitingForNextPrev , PreviousSong        , WaitingForNextPrev , dec_song_counter         , msm::front::none     >,
 msm::front::Row < WaitingForNextPrev , NextSong            , WaitingForNextPrev , inc_song_counter         , msm::front::none     >,
            //   +--------------------+---------------------+--------------------+--------------------------+----------------------+
            _row < WaitingForEnd      , EndPlay             , PlayingExit                                                          >,
            //   +--------------------+---------------------+--------------------+--------------------------+----------------------+
 msm::front::Row < NoForward          , EastPressed         , ForwardPressed      , msm::front::none        , fast_fwd_ok          >,
 msm::front::Row < ForwardPressed     , EastReleased        , NoForward           , send_NextSong           , msm::front::none     >,
           a_row < ForwardPressed     , ForwardTimer        , FastForward         , &fsm::do_fast_forward                          >,
           a_row < FastForward        , ForwardTimer        , FastForward         , &fsm::do_fast_forward                          >,
            _row < FastForward        , EastReleased        , NoForward                                                            >,
            //   +--------------------+---------------------+---------------------+--------------------------+----------------------+
            _row < StdDisplay         , PlayingMiddleButton , SetPosition                                                          >,
            _row < SetPosition        , StartSong           , StdDisplay                                                           >,
            _row < SetPosition        , PlayingMiddleButton , SetMark                                                              >,
            _row < SetMark            , PlayingMiddleButton , StdDisplay                                                           >,
            _row < SetMark            , StartSong           , StdDisplay                                                           >
            //   +--------------------+---------------------+---------------------+--------------------------+----------------------+
    > {};
    PlayingMode_():
    m_SongIndex(1),
    // for simplicity we decide there are 5 songs
    m_NumberOfSongs(5){}

    int get_current_song(){return m_SongIndex;}
    int m_SongIndex;
    int m_NumberOfSongs;

};
typedef msm::back::state_machine<PlayingMode_> PlayingMode;

#endif // PLAYING_MODE_HPP
