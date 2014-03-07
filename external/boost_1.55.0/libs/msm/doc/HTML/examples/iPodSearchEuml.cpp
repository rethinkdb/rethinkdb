// Copyright 2010 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// same as iPodSearch.cpp but using eUML
// requires boost >= v1.40 because using mpl::string

#include <vector>
#include <iostream>

#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/euml/euml.hpp>
#include <boost/msm/front/euml/stl.hpp>

using namespace std;
using namespace boost::msm::front::euml;
namespace msm = boost::msm;
namespace mpl = boost::mpl;

// how long the timer will ring when countdown elapsed.
#define RINGING_TIME 5

namespace  // Concrete FSM implementation
{
    // events
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(std::string,m_song)
    BOOST_MSM_EUML_ATTRIBUTES((attributes_ << m_song ), OneSongDef)
    struct OneSong_impl : euml_event<OneSong_impl>,OneSongDef
    {
        OneSong_impl(){}
        OneSong_impl(const string& asong)
        {
            get_attribute(m_song)=asong;
        }
        OneSong_impl(const char* asong)
        {
            get_attribute(m_song)=asong;
        }
        OneSong_impl(const OneSong_impl& asong)
        {
            get_attribute(m_song)=asong.get_attribute(m_song);
        }
        const string& get_data() const {return get_attribute(m_song);}
    };
    OneSong_impl const OneSong;

    // attribute definitions
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(vector<OneSong_impl>,m_src_container)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(vector<OneSong_impl>,m_tgt_container)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(std::string,m_letters)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(vector<OneSong_impl>::iterator,m_src_it)

    // the same attribute name can be reused
    BOOST_MSM_EUML_ATTRIBUTES((attributes_ << m_song ), NotFoundDef)
    BOOST_MSM_EUML_EVENT_WITH_ATTRIBUTES(NotFound,NotFoundDef)

    BOOST_MSM_EUML_ATTRIBUTES((attributes_ << m_song ), FoundDef)
    BOOST_MSM_EUML_EVENT_WITH_ATTRIBUTES(Found,FoundDef)

    BOOST_MSM_EUML_EVENT(Done)

    // Concrete FSM implementation 

    // The list of FSM states
    BOOST_MSM_EUML_STATE(( (push_back_(fsm_(m_tgt_container),event_(m_song)) 
                            ,process_(Done)),
                           no_action ),Insert)

    BOOST_MSM_EUML_STATE(( if_then_else_( string_find_(event_(m_song),state_(m_letters)) != Npos_<string>() ,
                                          process2_(Found,event_(m_song)),
                                          process2_(NotFound,event_(m_song)) ) ,
                           no_action, 
                           attributes_ << m_letters ),StringFind)

    BOOST_MSM_EUML_STATE(( if_then_( state_(m_src_it) != end_(fsm_(m_src_container)),
                                     process2_(OneSong,*(state_(m_src_it)++)) ),
                           no_action, 
                           attributes_ << m_src_it ),Foreach)

    // replaces the old transition table
    BOOST_MSM_EUML_TRANSITION_TABLE((
          StringFind  == Foreach     + OneSong ,
          Insert      == StringFind  + Found  ,
          Foreach     == StringFind  + NotFound ,
          Foreach     == Insert      + Done
          //  +------------------------------------------------------------------------------+
          ),transition_table )

    BOOST_MSM_EUML_ACTION(Log_No_Transition)
    {
        template <class FSM,class Event>
        void operator()(Event const& e,FSM&,int state)
        {
            std::cout << "no transition from state " << state
                << " on event " << typeid(e).name() << std::endl;
        }
    };
    // create a state machine "on the fly"
    BOOST_MSM_EUML_DECLARE_STATE_MACHINE((     transition_table, //STT
                                            init_ << Foreach, // Init
                                            (
                                            clear_(fsm_(m_src_container)), //clear source
                                            clear_(fsm_(m_tgt_container)), //clear results
                                            push_back_(fsm_(m_src_container), 
                                                       String_<mpl::string<'Let ','it ','be'> >()),//add a song
                                            push_back_(fsm_(m_src_container),
                                                       String_<mpl::string<'Yell','ow s','ubma','rine'> >()),//add a song
                                            push_back_(fsm_(m_src_container),
                                                       String_<mpl::string<'Twis','t an','d Sh','out'> >()),//add a song
                                            push_back_(fsm_(m_src_container),
                                                       String_<mpl::string<'She ','love','s yo','u'> >()),//add a song
                                            attribute_(substate_(Foreach()),m_src_it)
                                                = begin_(fsm_(m_src_container)) //set the search begin
                                            ), // Entry
                                            no_action, // Exit
                                            attributes_ << m_src_container // song list
                                                        << m_tgt_container, // result
                                            configure_<< no_configure_,
                                            Log_No_Transition
                                        ),
                                        iPodSearch_) //fsm name


    // choice of back-end
    typedef msm::back::state_machine<iPodSearch_> iPodSearch;

    void test()
    {        
        iPodSearch search;

        // look for "She Loves You" using the first letters
        search.get_state<BOOST_MSM_EUML_STATE_NAME(StringFind)&>().get_attribute(m_letters)="Sh";// will find 2 songs

        // needed to start the highest-level SM. This will call on_entry and mark the start of the SM
        search.start(); 
        // display all the songs
        for (vector<OneSong_impl>::const_iterator it = search.get_attribute(m_tgt_container).begin(); 
             it != search.get_attribute(m_tgt_container).end();++it)
        {
            cout << "candidate song:" << (*it).get_attribute(m_song) << endl;
        }

        cout << "search using more letters" << endl;
        // look for "She Loves You" using more letters
        search.get_state<BOOST_MSM_EUML_STATE_NAME(StringFind)&>().get_attribute(m_letters)="She";// will find 1 song
        search.start(); 
        // display all the songs
        for (vector<OneSong_impl>::const_iterator it = search.get_attribute(m_tgt_container).begin(); 
             it != search.get_attribute(m_tgt_container).end();++it)
        {
            cout << "candidate song:" << (*it).get_attribute(m_song) << endl;
        }
    }
}

int main()
{
    test();
    return 0;
}
