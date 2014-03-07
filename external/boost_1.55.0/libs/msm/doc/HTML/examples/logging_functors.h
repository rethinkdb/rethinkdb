// Copyright 2010 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef LOGGING_FUNCTORS
#define LOGGING_FUNCTORS

BOOST_MSM_EUML_ACTION(Empty_Entry)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "entering: Empty" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(Empty_Exit)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "leaving: Empty" << std::endl;
    }
};

BOOST_MSM_EUML_ACTION(Open_Entry)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "entering: Open" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(Open_Exit)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "leaving: Open" << std::endl;
    }
};

BOOST_MSM_EUML_ACTION(Stopped_Entry)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "entering: Stopped" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(Stopped_Exit)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "leaving: Stopped" << std::endl;
    }
};

BOOST_MSM_EUML_ACTION(AllOk_Entry)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "starting: AllOk" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(AllOk_Exit)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "finishing: AllOk" << std::endl;
    }
};

BOOST_MSM_EUML_ACTION(ErrorMode_Entry)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "starting: ErrorMode" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(ErrorMode_Exit)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "finishing: ErrorMode" << std::endl;
    }
};

BOOST_MSM_EUML_ACTION(Playing_Entry)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "entering: Playing" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(Playing_Exit)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "leaving: Playing" << std::endl;
    }
};

BOOST_MSM_EUML_ACTION(Song1_Entry)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "starting: First song" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(Song1_Exit)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "finishing: First Song" << std::endl;
    }
};

BOOST_MSM_EUML_ACTION(Song2_Entry)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "starting: Second song" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(Song2_Exit)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "finishing: Second Song" << std::endl;
    }
};

BOOST_MSM_EUML_ACTION(Song3_Entry)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "starting: Third song" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(Song3_Exit)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "finishing: Third Song" << std::endl;
    }
};

BOOST_MSM_EUML_ACTION(Region2State1_Entry)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "starting: Region2State1" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(Region2State1_Exit)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "finishing: Region2State1" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(Region2State2_Entry)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "starting: Region2State2" << std::endl;
    }
};
BOOST_MSM_EUML_ACTION(Region2State2_Exit)
{
    template <class Event,class FSM,class STATE>
    void operator()(Event const&,FSM&,STATE& )
    {
        std::cout << "finishing: Region2State2" << std::endl;
    }
};
// transition actions for Playing
BOOST_MSM_EUML_ACTION(start_next_song)
{
    template <class FSM,class EVT,class SourceState,class TargetState>
    void operator()(EVT const& ,FSM&,SourceState& ,TargetState& )
    {
        std::cout << "Playing::start_next_song" << endl;
    }
};
BOOST_MSM_EUML_ACTION(start_prev_song)
{
    template <class FSM,class EVT,class SourceState,class TargetState>
    void operator()(EVT const& ,FSM&,SourceState& ,TargetState& )
    {
        std::cout << "Playing::start_prev_song" << endl;
    }
};

// transition actions
BOOST_MSM_EUML_ACTION(start_playback)
{
    template <class FSM,class EVT,class SourceState,class TargetState>
    void operator()(EVT const& ,FSM&,SourceState& ,TargetState& )
    {
        cout << "player::start_playback" << endl;
    }
};
BOOST_MSM_EUML_ACTION(open_drawer)
{
    template <class FSM,class EVT,class SourceState,class TargetState>
    void operator()(EVT const& ,FSM&,SourceState& ,TargetState& )
    {
        cout << "player::open_drawer" << endl;
    }
};
BOOST_MSM_EUML_ACTION(close_drawer)
{
    template <class FSM,class EVT,class SourceState,class TargetState>
    void operator()(EVT const& ,FSM&,SourceState& ,TargetState& )
    {
        cout << "player::close_drawer" << endl;
    }
};
BOOST_MSM_EUML_ACTION(store_cd_info)
{
    template <class FSM,class EVT,class SourceState,class TargetState>
    void operator()(EVT const&, FSM& ,SourceState& ,TargetState& )
    {
        cout << "player::store_cd_info" << endl;
        // it is now easy to use the message queue. 
        // alternatively to the proces_ in the transition table, we could write:
        // fsm.process_event(play());
    }
};
BOOST_MSM_EUML_ACTION(stop_playback)
{
    template <class FSM,class EVT,class SourceState,class TargetState>
    void operator()(EVT const& ,FSM&,SourceState& ,TargetState& )
    {
        cout << "player::stop_playback" << endl;
    }
};
BOOST_MSM_EUML_ACTION(pause_playback)
{
    template <class FSM,class EVT,class SourceState,class TargetState>
    void operator()(EVT const& ,FSM&,SourceState& ,TargetState& )
    {
        cout << "player::pause_playback" << endl;
    }
};
BOOST_MSM_EUML_ACTION(resume_playback)
{
    template <class FSM,class EVT,class SourceState,class TargetState>
    void operator()(EVT const& ,FSM&,SourceState& ,TargetState& )
    {
        cout << "player::resume_playback" << endl;
    }
};
BOOST_MSM_EUML_ACTION(stop_and_open)
{
    template <class FSM,class EVT,class SourceState,class TargetState>
    void operator()(EVT const& ,FSM&,SourceState& ,TargetState& )
    {
        cout << "player::stop_and_open" << endl;
    }
};
BOOST_MSM_EUML_ACTION(stopped_again)
{
    template <class FSM,class EVT,class SourceState,class TargetState>
    void operator()(EVT const& ,FSM&,SourceState& ,TargetState& )
    {
        cout << "player::stopped_again" << endl;
    }
};

BOOST_MSM_EUML_ACTION(report_error)
{
    template <class FSM,class EVT,class SourceState,class TargetState>
    void operator()(EVT const& ,FSM&,SourceState& ,TargetState& )
    {
        cout << "player::report_error" << endl;
    }
};

BOOST_MSM_EUML_ACTION(report_end_error)
{
    template <class FSM,class EVT,class SourceState,class TargetState>
    void operator()(EVT const& ,FSM&,SourceState& ,TargetState& )
    {
        cout << "player::report_end_error" << endl;
    }
};
BOOST_MSM_EUML_ACTION(internal_action1)
{
    template <class FSM,class EVT,class SourceState,class TargetState>
    void operator()(EVT const&, FSM& ,SourceState& ,TargetState& )
    {
        cout << "Open::internal action1" << endl;
    }
};
BOOST_MSM_EUML_ACTION(internal_action2)
{
    template <class FSM,class EVT,class SourceState,class TargetState>
    void operator()(EVT const&, FSM& ,SourceState& ,TargetState& )
    {
        cout << "Open::internal action2" << endl;
    }
};
BOOST_MSM_EUML_ACTION(internal_action)
{
    template <class FSM,class EVT,class SourceState,class TargetState>
    void operator()(EVT const&, FSM& ,SourceState& ,TargetState& )
    {
        cout << "Open::internal action" << endl;
    }
};
enum DiskTypeEnum
{
    DISK_CD=0,
    DISK_DVD=1
};

// Handler called when no_transition detected
BOOST_MSM_EUML_ACTION(Log_No_Transition)
{
    template <class FSM,class Event>
    void operator()(Event const& e,FSM&,int state)
    {
        std::cout << "no transition from state " << state
            << " on event " << typeid(e).name() << std::endl;
    }
};

BOOST_MSM_EUML_ACTION(internal_guard1)
{
    template <class FSM,class EVT,class SourceState,class TargetState>
    bool operator()(EVT const&, FSM& ,SourceState& ,TargetState& )
    {
        cout << "Open::internal guard1" << endl;
        return false;
    }
};
BOOST_MSM_EUML_ACTION(internal_guard2)
{
    template <class FSM,class EVT,class SourceState,class TargetState>
    bool operator()(EVT const&, FSM& ,SourceState& ,TargetState& )
    {
        cout << "Open::internal guard2" << endl;
        return false;
    }
};
#endif // LOGGING_FUNCTORS
