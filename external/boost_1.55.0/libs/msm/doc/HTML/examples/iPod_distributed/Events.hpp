// Copyright 2010 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef EVENTS_HPP
#define EVENTS_HPP

//flags 
struct MenuActive{};
// hardware-generated events
struct Hold {};
struct NoHold {};
struct SouthPressed {};
struct SouthReleased {};
struct MiddleButton {};
struct EastPressed{};
struct EastReleased{};
struct Off {};
struct MenuButton {};

// internally used events
struct PlayPause {};
struct EndPlay {};
struct CloseMenu 
{
    template<class EVENT>
    CloseMenu(EVENT const &) {}
};
struct OnOffTimer {};
struct MenuMiddleButton {};
struct SelectSong {};
struct SongFinished {};
struct StartSong 
{
    StartSong (int song_index):m_Selected(song_index){}
    int m_Selected;
};
struct PreviousSong{};
struct NextSong{};
struct NextSongDerived : public NextSong{};
struct ForwardTimer{};
struct PlayingMiddleButton{};

#endif // EVENTS_HPP
