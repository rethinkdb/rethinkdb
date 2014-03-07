// Copyright 2010 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <set>
#include <string>
#include <iostream>
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>

using namespace std;
namespace msm = boost::msm;

namespace  // Concrete FSM implementation
{
    // events
    struct OneSong 
    {
        OneSong(string const& asong):m_Song(asong){}
        const string& get_data() const {return m_Song;}
    private:
        string m_Song;
    };
    template <class DATA>
    struct NotFound
    {
        DATA get_data() const {return m_Data;}
        DATA m_Data;
    };
    template <class DATA>
    struct Found
    {
        DATA get_data() const {return m_Data;}
        DATA m_Data;
    };
    struct Done {};

    template <class Container,class BASE_TYPE,class FSMType>
    struct Insert : public boost::msm::front::state<BASE_TYPE,boost::msm::front::sm_ptr> 
    {
        template <class Event,class FSM>
        void on_entry(Event const& evt,FSM& ) 
        {
            //TODO other containers
            if (m_Cont)
            {
                m_Cont->insert(evt.get_data());
            }
            m_fsm->process_event(Done());
        }
        void set_sm_ptr(FSMType* fsm){m_fsm=fsm;} 
        void set_container(Container* cont){m_Cont=cont;}
        Container*  m_Cont;

    private:
        FSMType*    m_fsm;
    }; 
    template <class BASE_TYPE,class FSMType>
    struct StringFind : public boost::msm::front::state<BASE_TYPE,boost::msm::front::sm_ptr> 
    {  
        template <class Event,class FSM>
        void on_entry(Event const& evt,FSM& ) 
        {
            //TODO other containers
            // if the element in the event is found
            if (evt.get_data().find(m_Cont) != std::string::npos )
            {
                Found<std::string> res;
                res.m_Data = evt.get_data();
                m_fsm->process_event(res);
            }
            // data not found
            else
            {
                NotFound<std::string> res;
                res.m_Data = evt.get_data();
                m_fsm->process_event(res);
            }
        }
        void set_sm_ptr(FSMType* fsm){m_fsm=fsm;} 
        void set_container(const char* cont){m_Cont=cont;}
    private:
        std::string   m_Cont;
        FSMType*      m_fsm;
    };
    template <class EventType,class Container,class BASE_TYPE,class FSMType>
    struct Foreach : public boost::msm::front::state<BASE_TYPE,boost::msm::front::sm_ptr> 
    {
        template <class Event,class FSM>
        void on_entry(Event const& evt,FSM& ) 
        {
            //TODO other containers
            if (!m_Cont.empty())
            {
                typename Container::value_type next_event = *m_Cont.begin();
                m_Cont.erase(m_Cont.begin());
                m_fsm->process_event(EventType(next_event));
            }
        }
        void set_sm_ptr(FSMType* fsm){m_fsm=fsm;} 
        void set_container(Container* cont){m_Cont=*cont;}

    private:
        Container   m_Cont;
        FSMType*    m_fsm;
    }; 


    // Concrete FSM implementation 
    struct iPodSearch_ : public msm::front::state_machine_def<iPodSearch_>
    {
        typedef msm::back::state_machine<iPodSearch_> iPodSearch;
        // The list of FSM states
        typedef std::set<std::string> Songset;
        typedef Insert<Songset,boost::msm::front::default_base_state,iPodSearch> MyInsert; 
        typedef StringFind<boost::msm::front::default_base_state,iPodSearch> MyFind;
        typedef Foreach<OneSong,Songset,boost::msm::front::default_base_state,iPodSearch> MyForeach;

        // the initial state of the player SM. Must be defined
        typedef MyForeach initial_state;

        // transition actions

        // guard conditions

        typedef iPodSearch_ fsm; // makes transition table cleaner

        // Transition table for player
        struct transition_table : mpl::vector4<
            //     Start       Event              Next         Action                Guard
            //    +-----------+------------------+------------+---------------------+----------------------+
            _row < MyForeach  , OneSong          , MyFind                                                  >,
            _row < MyFind     , NotFound<string> , MyForeach                                               >,
            _row < MyFind     , Found<string>    , MyInsert                                                >,
            _row < MyInsert   , Done             , MyForeach                                               >
            //    +-----------+------------------+------------+---------------------+----------------------+
        > {};
        iPodSearch_():m_AllSongs(),m_ResultSearch()
        {
            // add a few songs for testing
            m_AllSongs.insert("Let it be");
            m_AllSongs.insert("Yellow submarine");
            m_AllSongs.insert("Twist and Shout");
            m_AllSongs.insert("She Loves You");
        }
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& fsm) 
        {
            fsm.template get_state<MyForeach&>().set_container(&m_AllSongs);
            fsm.template get_state<MyInsert&>().set_container(&m_ResultSearch);
        }
        const Songset& get_result(){return m_ResultSearch;}
        void reset_search(){m_ResultSearch.clear();}

        // Replaces the default no-transition response.
        template <class FSM,class Event>
        void no_transition(Event const& e, FSM&,int state)
        {
            std::cout << "no transition from state " << state
                << " on event " << typeid(e).name() << std::endl;
        }

    private:
        Songset m_AllSongs;
        Songset m_ResultSearch;
    };
    typedef msm::back::state_machine<iPodSearch_> iPodSearch;

   
    void test()
    {
        iPodSearch search;
        // look for "She Loves You" using the first letters
        search.get_state<iPodSearch::MyFind*>()->set_container("Sh");// will find 2 songs

        // needed to start the highest-level SM. This will call on_entry and mark the start of the SM
        search.start(); 
        // display all the songs
        const iPodSearch::Songset& res = search.get_result();
        for (iPodSearch::Songset::const_iterator it = res.begin();it != res.end();++it)
        {
            cout << "candidate song:" << *it << endl;
        }
        cout << "search using more letters" << endl;
        // look for "She Loves You" using more letters
        search.reset_search();
        search.get_state<iPodSearch::MyFind*>()->set_container("She");// will find 1 song
        search.start(); 
        const iPodSearch::Songset& res2 = search.get_result();
        for (iPodSearch::Songset::const_iterator it = res2.begin();it != res2.end();++it)
        {
            cout << "candidate song:" << *it << endl;
        }

    }
}

int main()
{
    test();
    return 0;
}
