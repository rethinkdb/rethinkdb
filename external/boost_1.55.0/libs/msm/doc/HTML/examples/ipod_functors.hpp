// Copyright 2010 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef IPOD_FUNCTORS_HPP
#define IPOD_FUNCTORS_HPP
#include <boost/msm/front/euml/euml.hpp>

BOOST_MSM_EUML_ACTION(NotHolding_Entry)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "entering: NotHolding" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(Holding_Entry)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "entering: Holding" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(NotPlaying_Entry)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "entering: NotPlaying" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(NoMenuMode_Entry)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "entering: NoMenuMode" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(NoOnOffButton_Entry)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "entering: NoOnOffButton" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(OffDown_Entry)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "entering: OffDown" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(PlayerOff_Entry)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "entering: PlayerOff" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(CheckMiddleButton_Entry)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "entering: CheckMiddleButton" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(Playing_Entry)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM& fsm,STATE& )
    {
        std::cout << "entering: Playing" << std::endl;
        std::cout << "playing song:" << fsm.get_attribute(m_SongIndex) << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(WaitingForNextPrev_Entry)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "entering: WaitingForNextPrev" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(Paused_Entry)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "entering: Paused" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(WaitingForEnd_Entry)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "entering: OffDown" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(NoForward_Entry)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "entering: NoForward" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(ForwardPressed_Entry)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "entering: ForwardPressed" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(ForwardPressed_Exit)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "leaving: ForwardPressed" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(FastForward_Entry)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "entering: FastForward" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(FastForward_Exit)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "leaving: FastForward" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(StdDisplay_Entry)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "entering: StdDisplay" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(SetPosition_Entry)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "entering: SetPosition" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(SetMark_Entry)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "entering: SetMark" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(PlayingExit_Entry)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "entering: PlayingExit" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(WaitingForSongChoice_Entry)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "entering: WaitingForSongChoice" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(StartCurrentSong_Entry)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "entering: StartCurrentSong" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(MenuExit_Entry)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "entering: MenuExit" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(show_selected_song)
{
    template <class FSM,class EVT,class SourceState,class TargetState>
    void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
    {
        std::cout << "selecting song:" << fsm.get_attribute(m_SongIndex) << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(do_fast_forward)
{
    template <class FSM,class EVT,class SourceState,class TargetState>
    void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
    {
        std::cout << "moving song forward..." << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(show_playing_song)
{
    template <class FSM,class EVT,class SourceState,class TargetState>
    void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
    {
        std::cout << "playing song:" << fsm.get_attribute(m_SongIndex) << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(show_player_off)
{
    template <class FSM,class EVT,class SourceState,class TargetState>
    void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
    {
        std::cout << "turning player off" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(show_player_on)
{
    template <class FSM,class EVT,class SourceState,class TargetState>
    void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
    {
        std::cout << "turning player on" << std::endl;
    }
};
#endif
