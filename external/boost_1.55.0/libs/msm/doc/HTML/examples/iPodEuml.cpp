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
// we need more than the default 20 states
#define FUSION_MAX_VECTOR_SIZE 20
// we need more than the default 20 transitions
#include "boost/mpl/vector/vector50.hpp"
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/euml/euml.hpp>


using namespace std;
using namespace boost::msm::front::euml;
namespace msm = boost::msm;

// attribute names and types
BOOST_MSM_EUML_DECLARE_ATTRIBUTE(int,m_Selected)
BOOST_MSM_EUML_DECLARE_ATTRIBUTE(int,m_SongIndex)
BOOST_MSM_EUML_DECLARE_ATTRIBUTE(int,m_NumberOfSongs)
#include "ipod_functors.hpp"


namespace  // Concrete FSM implementation
{
    //flags 
    BOOST_MSM_EUML_FLAG(MenuActive)
    BOOST_MSM_EUML_FLAG(NoFastFwd)
    // hardware-generated events
    BOOST_MSM_EUML_EVENT(Hold)
    BOOST_MSM_EUML_EVENT(NoHold)
    BOOST_MSM_EUML_EVENT(SouthPressed)
    BOOST_MSM_EUML_EVENT(SouthReleased)
    BOOST_MSM_EUML_EVENT(MiddleButton)
    BOOST_MSM_EUML_EVENT(EastPressed)
    BOOST_MSM_EUML_EVENT(EastReleased)
    BOOST_MSM_EUML_EVENT(Off)
    BOOST_MSM_EUML_EVENT(MenuButton)
    // internally defined events
    BOOST_MSM_EUML_EVENT(PlayPause)
    BOOST_MSM_EUML_EVENT(EndPlay)
    struct CloseMenu_impl : euml_event<CloseMenu_impl>
    {
        CloseMenu_impl(){}//defined only for stt
        template<class EVENT>
        CloseMenu_impl(EVENT const &) {}
    };
    CloseMenu_impl const CloseMenu;
    BOOST_MSM_EUML_EVENT(OnOffTimer)
    BOOST_MSM_EUML_EVENT(MenuMiddleButton)
    BOOST_MSM_EUML_EVENT(SelectSong)
    BOOST_MSM_EUML_EVENT(SongFinished)

    BOOST_MSM_EUML_ATTRIBUTES((attributes_ << m_Selected ), StartSongAttributes)
    BOOST_MSM_EUML_EVENT_WITH_ATTRIBUTES(StartSong,StartSongAttributes)
    BOOST_MSM_EUML_EVENT(PreviousSong)
    BOOST_MSM_EUML_EVENT(NextSong)
    BOOST_MSM_EUML_EVENT(ForwardTimer)
    BOOST_MSM_EUML_EVENT(PlayingMiddleButton)

    // Concrete iPod implementation 
    // The list of iPod states
    BOOST_MSM_EUML_STATE(( NotHolding_Entry ),NotHolding)
    BOOST_MSM_EUML_INTERRUPT_STATE(( NoHold,Holding_Entry ),Holding)
    BOOST_MSM_EUML_STATE(( NotPlaying_Entry ),NotPlaying)
    BOOST_MSM_EUML_STATE(( NoMenuMode_Entry ),NoMenuMode)
    BOOST_MSM_EUML_STATE(( NoOnOffButton_Entry ),NoOnOffButton)
    BOOST_MSM_EUML_STATE(( OffDown_Entry ),OffDown)
    BOOST_MSM_EUML_STATE(( PlayerOff_Entry ),PlayerOff)
    BOOST_MSM_EUML_STATE(( CheckMiddleButton_Entry ),CheckMiddleButton)

    // Concrete PlayingMode_ implementation 
    // The list of PlayingMode_ states
    BOOST_MSM_EUML_STATE(( Playing_Entry ),Playing)
    BOOST_MSM_EUML_STATE(( WaitingForNextPrev_Entry ),WaitingForNextPrev)
    BOOST_MSM_EUML_STATE(( Paused_Entry ),Paused)
    BOOST_MSM_EUML_STATE(( WaitingForEnd_Entry ),WaitingForEnd)
    BOOST_MSM_EUML_STATE(( NoForward_Entry ),NoForward)
    BOOST_MSM_EUML_STATE(( ForwardPressed_Entry,ForwardPressed_Exit ),ForwardPressed)
    BOOST_MSM_EUML_STATE(( FastForward_Entry,FastForward_Exit ),FastForward)
    BOOST_MSM_EUML_STATE(( StdDisplay_Entry ),StdDisplay)
    BOOST_MSM_EUML_STATE(( SetPosition_Entry ),SetPosition)
    BOOST_MSM_EUML_STATE(( SetMark_Entry ),SetMark)
    BOOST_MSM_EUML_EXIT_STATE(( EndPlay,PlayingExit_Entry ),PlayingExit)

    //stt
    BOOST_MSM_EUML_TRANSITION_TABLE((
        //  +------------------------------------------------------------------------------+
        Paused             == Playing           + PlayPause                                            ,
        Paused             == Playing           + Off                                                  ,
        Playing            == Playing           + StartSong 
                             / (if_then_(event_(m_Selected) > Int_<0>() && 
                                         event_(m_Selected) < fsm_(m_NumberOfSongs),
                                         fsm_(m_SongIndex) = event_(m_Selected) ),show_selected_song)   ,
        Playing            == Playing + SongFinished
                             / (if_then_else_(++fsm_(m_SongIndex) <= fsm_(m_NumberOfSongs),  /*if*/
                                             show_playing_song,                               /*then*/
                                             (fsm_(m_SongIndex)=Int_<1>(),process_(EndPlay))/*else*/ ) )  ,
        Playing            == Paused            + PlayPause                                             ,
        Playing            == Paused            + StartSong
                             / (if_then_(event_(m_Selected) > Int_<0>() && 
                                         event_(m_Selected) < fsm_(m_NumberOfSongs),
                                         fsm_(m_SongIndex) = event_(m_Selected) ),show_selected_song)    ,
        WaitingForNextPrev == WaitingForNextPrev+ PreviousSong                     
                             /( if_then_else_(--fsm_(m_SongIndex) > Int_<0>(),                  /*if*/
                                              show_playing_song,                                /*then*/
                                              (fsm_(m_SongIndex)=Int_<1>(),process_(EndPlay)) /*else*/ ) ) ,
        WaitingForNextPrev == WaitingForNextPrev+ NextSong
                               / (if_then_else_(++fsm_(m_SongIndex) <= fsm_(m_NumberOfSongs),      /*if*/
                                                show_playing_song,                               /*then*/
                                                (fsm_(m_SongIndex)=Int_<1>(),process_(EndPlay))  /*else*/ ) ),

        PlayingExit        == WaitingForEnd     + EndPlay                                                ,
        ForwardPressed     == NoForward         + EastPressed [!is_flag_(NoFastFwd)]                   ,
        NoForward          == ForwardPressed    + EastReleased      / process_(NextSong)               ,
        FastForward        == ForwardPressed    + ForwardTimer    / do_fast_forward                    ,
        FastForward        == FastForward       + ForwardTimer    / do_fast_forward                    ,
        FastForward        == NoForward         + EastReleased                                           ,
        SetPosition        == StdDisplay        + PlayingMiddleButton                                    ,
        StdDisplay         == SetPosition       + StartSong                                              ,
        SetMark            == SetPosition       + PlayingMiddleButton                                    ,
        StdDisplay         == SetMark           + PlayingMiddleButton                                    ,
        StdDisplay         == SetMark           + StartSong  
        //  +------------------------------------------------------------------------------+
        ),playingmode_transition_table )

    BOOST_MSM_EUML_DECLARE_STATE_MACHINE( (playingmode_transition_table, //STT
                                        init_ << Playing << WaitingForNextPrev << WaitingForEnd 
                                              << NoForward << StdDisplay, // Init States
                                        fsm_(m_NumberOfSongs)=Int_<5>(), // entry
                                        no_action, // exit
                                        attributes_ << m_SongIndex << m_NumberOfSongs, //attributes
                                        configure_<< NoFastFwd // Flags, Deferred events, configuration
                                        ),PlayingMode_)

    // choice of back-end
    typedef msm::back::state_machine<PlayingMode_> PlayingMode_type;
    PlayingMode_type const PlayingMode;

    // Concrete MenuMode_ implementation 
    // The list of MenuMode_ states
    BOOST_MSM_EUML_STATE(( WaitingForSongChoice_Entry ),WaitingForSongChoice)
    BOOST_MSM_EUML_STATE(( StartCurrentSong_Entry ),StartCurrentSong)
    BOOST_MSM_EUML_EXIT_STATE(( CloseMenu,MenuExit_Entry ),MenuExit)

    //stt
    BOOST_MSM_EUML_TRANSITION_TABLE((
        //  +------------------------------------------------------------------------------+
        StartCurrentSong   == WaitingForSongChoice  + MenuMiddleButton  ,
        MenuExit           == StartCurrentSong      + SelectSong 
        //  +------------------------------------------------------------------------------+
        ),menumode_transition_table )

    BOOST_MSM_EUML_DECLARE_STATE_MACHINE( (menumode_transition_table, //STT
                                        init_ << WaitingForSongChoice, // Init States
                                        no_action, // entry
                                        no_action, // exit
                                        attributes_ << no_attributes_, //attributes
                                        configure_<< MenuActive // Flags, Deferred events, configuration
                                        ),MenuMode_)

    typedef msm::back::state_machine<MenuMode_> MenuMode_type;
    MenuMode_type const MenuMode;

    // iPod stt
    BOOST_MSM_EUML_TRANSITION_TABLE((
        //  +------------------------------------------------------------------------------+
        Holding           == NotHolding                 + Hold                        ,
        NotHolding        == Holding                    + NoHold                      ,
        PlayingMode       == NotPlaying                 + PlayPause                   ,
        NotPlaying        == exit_pt_(PlayingMode,PlayingExit)  + EndPlay    
                                / process_(MenuButton)                                    ,
        MenuMode          == NoMenuMode                 + MenuButton                  ,
        NoMenuMode        == exit_pt_(MenuMode,MenuExit)+ CloseMenu    
                                / process2_(StartSong,Int_<5>())                          ,
        OffDown           == NoOnOffButton              + SouthPressed                ,
        NoOnOffButton     == OffDown                    + SouthReleased 
                                / process_(PlayPause)                                     ,
        PlayerOff         == OffDown                    + OnOffTimer     
                                / (show_player_off,process_(Off))                       ,
        NoOnOffButton     == PlayerOff                  + SouthPressed / show_player_on ,
        NoOnOffButton     == PlayerOff                  + NoHold / show_player_on   ,
        CheckMiddleButton == CheckMiddleButton          + MiddleButton 
                               [is_flag_(MenuActive)] / process_(PlayingMiddleButton)   ,
        CheckMiddleButton == CheckMiddleButton + MiddleButton
                               [!is_flag_(MenuActive)] / process_(PlayingMiddleButton) 
        //  +------------------------------------------------------------------------------+
        ),ipod_transition_table )

    BOOST_MSM_EUML_DECLARE_STATE_MACHINE(( ipod_transition_table, //STT
                                        init_ << NotHolding << NotPlaying << NoMenuMode 
                                        << NoOnOffButton << CheckMiddleButton
                                        ),
                                      iPod_) //fsm name
    typedef msm::back::state_machine<iPod_> iPod;
   
    void test()
    {
        iPod sm;
        sm.start();
        // we first press Hold
        std::cout << "pressing hold" << std::endl;
        sm.process_event(Hold);
        // pressing a button is now ignored
        std::cout << "pressing a button" << std::endl;
        sm.process_event(SouthPressed);
        // or even one contained in a submachine
        sm.process_event(EastPressed);
        // no more holding
        std::cout << "no more holding, end interrupt event sent" << std::endl;
        sm.process_event(NoHold);
        std::cout << "pressing South button a short time" << std::endl;
        sm.process_event(SouthPressed);
        // we suppose a short pressing leading to playing a song
        sm.process_event(SouthReleased);
        // we move to the next song
        std::cout << "we move to the next song" << std::endl;
        sm.process_event(NextSong);
        // then back to no song => exit from playing, menu active
        std::cout << "we press twice the West button (simulated)=> end of playing" << std::endl;
        sm.process_event(PreviousSong);
        sm.process_event(PreviousSong);
        // even in menu mode, pressing play will start playing the first song
        std::cout << "pressing play/pause" << std::endl;
        sm.process_event(SouthPressed);
        sm.process_event(SouthReleased);
        // of course pausing must be possible
        std::cout << "pressing play/pause" << std::endl;
        sm.process_event(SouthPressed);
        sm.process_event(SouthReleased);
        std::cout << "pressing play/pause" << std::endl;
        sm.process_event(SouthPressed);
        sm.process_event(SouthReleased);
        // while playing, you can fast forward
        std::cout << "pressing East button a long time" << std::endl;
        sm.process_event(EastPressed);
        // let's suppose the timer just fired
        sm.process_event(ForwardTimer);
        sm.process_event(ForwardTimer);
        // end of fast forwarding
        std::cout << "releasing East button" << std::endl;
        sm.process_event(EastReleased);
        // we now press the middle button to set playing at a given position
        std::cout << "pressing Middle button, fast forwarding disabled" << std::endl;
        sm.process_event(MiddleButton);
        std::cout <<"pressing East button to fast forward" << std::endl;
        sm.process_event(EastPressed);
        // we switch off and on
        std::cout <<"switch off player" << std::endl;
        sm.process_event(SouthPressed);
        sm.process_event(OnOffTimer);
        std::cout <<"switch on player" << std::endl;    
        sm.process_event(SouthPressed);
    }
}

int main()
{
    test();
    return 0;
}
