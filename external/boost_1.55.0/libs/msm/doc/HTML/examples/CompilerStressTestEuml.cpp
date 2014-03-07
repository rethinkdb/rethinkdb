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
#include <list>
#include <set>
#include <map>
#include <iostream>

// we need more than the default 10 states
#define FUSION_MAX_VECTOR_SIZE 15

#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/euml/euml.hpp>
#include <boost/msm/front/euml/stl.hpp>

using namespace std;
using namespace boost::msm::front::euml;
namespace msm = boost::msm;

// how long the timer will ring when countdown elapsed.
#define RINGING_TIME 5

namespace  // Concrete FSM implementation
{
    // flag
    BOOST_MSM_EUML_FLAG(SomeFlag)

    // declares attributes with type and name. Can  be used anywhere after
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(std::string,m_song)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(int,m_song_id)

    // declare that a type inheriting from OneSongDef will get these 2 attributes
    BOOST_MSM_EUML_ATTRIBUTES((attributes_ << m_song << m_song_id ), OneSongDef)
    // events
    // this event is done "manually", not using any predefined macro
    struct OneSong_impl : euml_event<OneSong_impl>,OneSongDef
    {
        OneSong_impl(){}
        OneSong_impl(const string& asong)
        {
            get_attribute(m_song)=asong;
            get_attribute(m_song_id)=1;
        }
        OneSong_impl(const OneSong_impl& asong)
        {
            get_attribute(m_song)=asong.get_attribute(m_song);
            get_attribute(m_song_id)=1;
        }
        const string& get_data() const {return get_attribute(m_song);}
    };
    // declare an instance for use in the transition table
    OneSong_impl const OneSong;

    struct SongComparator : euml_action<SongComparator>
    {
        bool operator()(const OneSong_impl& lhs,const OneSong_impl& rhs)const
        {
            return lhs.get_data() == rhs.get_data();
        }
    };
    struct SongLessComparator : euml_action<SongLessComparator>
    {
        bool operator()(const OneSong_impl& lhs,const OneSong_impl& rhs)const
        {
            return lhs.get_data() < rhs.get_data();
        }
    };
    struct Comparator
    {
        template <class T>
        bool operator()(const T& lhs,const T& rhs)const
        {
            return lhs < rhs;
        }
    };
    struct RemoveDummy 
    {
        bool operator()(const OneSong_impl& lhs)const
        {
            return (lhs.get_attribute(m_song).compare(std::string("She-Dummy. Remove this one"))==0 );
        }
    };
    template <int val>
    struct LookFor
    {
        template <class T>
        bool operator()(const T& lhs)const
        {
            return lhs == val;
        }
    };
    template <int val>
    struct LessThan 
    {
        template <class T>
        bool operator()(const T& lhs)const
        {
            return lhs < val;
        }
    };
    BOOST_MSM_EUML_ACTION(SongDeleter)
    {
        bool operator()(const OneSong_impl& lhs)const
        {
            return lhs.get_data() == "Twist and Shout";
        }
    };
    struct Generator
    {
        int operator()()const
        {
            return 1;
        }
    };
    struct Print
    {
        template <class T>
        void operator()(const T& lhs)const
        {
            std::cout << "Song:" << lhs.get_data() << endl;
        }
    };
    BOOST_MSM_EUML_ATTRIBUTES((attributes_ << m_song ), NotFoundDef)
    // declare an event instance called NotFound with the defined attributes
    // these attributes can then be referenced anywhere (stt, state behaviors)
    BOOST_MSM_EUML_EVENT_WITH_ATTRIBUTES(NotFound,NotFoundDef)

    BOOST_MSM_EUML_ATTRIBUTES((attributes_ << m_song ), FoundDef)
    struct Found_impl : euml_event<Found_impl>,FoundDef
    {
        Found_impl(){}
        Found_impl (const string& data)
        {
            get_attribute(m_song)=data;
        }
        int foo()const {std::cout << "foo()" << std::endl; return 0;}
        int foo(int i)const {std::cout << "foo(int):" << i << std::endl; return 1;}
        int foo(int i,int j)const {std::cout << "foo(int,int):" << i <<"," << j << std::endl; return 2;}

    };
    Found_impl const Found;
    // some functions to call
    // this macro creates a functor and an eUML function wrapper. Now, foo_ can be used anywhere
    BOOST_MSM_EUML_METHOD(FoundFoo_ , foo , foo_ , int , int )

    template <class T>
    int do_print(T& t ) {std::cout << "print(T):" << typeid(T).name() << std::endl;return 1;}
    BOOST_MSM_EUML_FUNCTION(PrintState_ , do_print , print_ , int , int )

    BOOST_MSM_EUML_EVENT(Done)

    // Concrete FSM implementation 
    struct some_base 
    {
        int foobar()const {std::cout << "foobar()" << std::endl; return 0;}
        int foobar(int i)const {std::cout << "foobar(int):" << i << std::endl; return 1;}
        int foobar(int i,int j)const {std::cout << "foobar(int,int):" << i <<"," << j << std::endl; return 2;}
    };
    // some functions to call
    BOOST_MSM_EUML_METHOD(FooBar_ , foobar , foobar_ , int , int )

    // fsm attributes
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(vector<OneSong_impl>,m_src_container)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(list<OneSong_impl>,m_tgt_container)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(list<int>,m_var2)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(list<int>,m_var3)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(set<int>,m_var4)
    typedef std::map<int,int>  int_int_map;
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(int_int_map,m_var5)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(std::string,m_var6)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(std::string,m_var7)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(vector<int>,m_var8)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(vector<int>,m_var9)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(int,m_var10)

    // The list of FSM states
    BOOST_MSM_EUML_STATE(( ( insert_(fsm_(m_tgt_container),end_(fsm_(m_tgt_container)),
                                                append_(event_(m_song),fsm_(m_var7)) ),//foo_(event_,Int_<0>()) ,
                                                //foo_(event_,Int_<0>(),Int_<1>()),print_(state_),
                                                process_(Done/*,fsm_*/),if_then_(true_,true_) ),
                                       no_action
                                        ),Insert)

    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(std::string,m_letters)
    BOOST_MSM_EUML_STATE(( if_then_else_( (string_find_(event_(m_song),state_(m_letters),Size_t_<0>()) != Npos_<string>()&&
                                                      string_find_(event_(m_song),Char_<'S'>(),Size_t_<0>()) != Npos_<string>()&&
                                                      string_find_first_of_(event_(m_song),Char_<'S'>()) == Size_t_<0>() &&
                                                      string_compare_(event_(m_song),Int_<0>(),size_(event_(m_song)),event_(m_song)) == Int_<0>()
                                                      //&& is_flag_(SomeFlag(),fsm_())
                                                      //&& ( event_(m_song_id) == Int_<1>())
                                                      //&& string_find_(event_(m_song),String_<mpl::string<'Sh','e'> >())
                                                      //      != Npos_<string>()

                                                      ),
                                                     process2_(Found,
                                                        //string_insert_(event_(m_song),Size_t_<0>(),fsm_(m_var6)) ),
                                                        string_replace_(
                                                            string_assign_(
                                                                string_erase_(
                                                                    string_insert_(
                                                                        substr_(event_(m_song),Size_t_<1>()),
                                                                        Size_t_<0>(),
                                                                        Size_t_<1>(),
                                                                        Char_<'S'>()),
                                                                    Size_t_<0>(),
                                                                    Size_t_<1>() ), 
                                                                 event_(m_song) ), 
                                                            Size_t_<0>(),
                                                            Size_t_<1>(),
                                                            c_str_(fsm_(m_var6))
                                                            /*Size_t_<1>(),
                                                            Char_<'s'>()*/ ) ),
                                                     process2_(NotFound,event_(m_song),fsm_) ) ,
                                     no_action, 
                                     attributes_ << m_letters,
                                     configure_<< SomeFlag ),StringFind) 

    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(vector<OneSong_impl>::iterator,m_src_it)
    BOOST_MSM_EUML_STATE(( if_then_( (state_(m_src_it) != end_(fsm_(m_src_container)) &&
                                                //associative_find_(fsm_(m_var4),Int_<9>()) != end_(fsm_(m_var4))&&
                                                //associative_count_(fsm_(m_var4),Int_<9>()) == Size_t_<1>() &&
                                                //*associative_upper_bound_(fsm_(m_var4),Int_<8>()) == Int_<9>()&&
                                                //*associative_lower_bound_(fsm_(m_var4),Int_<9>()) == Int_<9>() &&
                                                //second_(associative_equal_range_(fsm_(m_var4),Int_<8>())) == associative_upper_bound_(fsm_(m_var4),Int_<8>()) &&
                                                //first_(associative_equal_range_(fsm_(m_var4),Int_<8>())) == associative_lower_bound_(fsm_(m_var4),Int_<8>())&&
                                                //second_(*associative_lower_bound_(fsm_(m_var5),Int_<0>())) == Int_<0>() && //map => pair as return
                                                //find_if_(begin_(fsm_(m_var4)),end_(fsm_(m_var4)),Predicate_<LookFor<8> >()) != end_(fsm_(m_var4))&&
                                                //*lower_bound_(begin_(fsm_(m_var4)),end_(fsm_(m_var4)),Int_<9>(),Predicate_<std::less<int> >()) == Int_<9>()&&
                                                //*upper_bound_(begin_(fsm_(m_var4)),end_(fsm_(m_var4)),Int_<8>(),Predicate_<std::less<int> >()) == Int_<9>() &&
                                                //second_(equal_range_(begin_(fsm_(m_var4)),end_(fsm_(m_var4)),Int_<8>())) 
                                                 //   == upper_bound_(begin_(fsm_(m_var4)),end_(fsm_(m_var4)),Int_<8>()) &&
                                                //first_(equal_range_(begin_(fsm_(m_var4)),end_(fsm_(m_var4)),Int_<9>(),Predicate_<std::less<int> >())) 
                                                //    == lower_bound_(begin_(fsm_(m_var4)),end_(fsm_(m_var4)),Int_<9>(),Predicate_<std::less<int> >())&&
                                                //binary_search_(begin_(fsm_(m_var4)),end_(fsm_(m_var4)),Int_<9>(),Predicate_<std::less<int> >())&&
                                                //binary_search_(begin_(fsm_(m_var4)),end_(fsm_(m_var4)),Int_<9>())&&
                                                //count_(begin_(fsm_(m_var4)),end_(fsm_(m_var4)),Int_<9>()) == Int_<1>()&&
                                                //count_if_(begin_(fsm_(m_var4)),end_(fsm_(m_var4)),Predicate_<LookFor<9> >()) == Int_<1>()&&
                                                //distance_(begin_(fsm_(m_var4)),end_(fsm_(m_var4))) == Int_<2>()&&
                                                //*min_element_(begin_(fsm_(m_var4)),end_(fsm_(m_var4)),Predicate_<std::less<int> >()) == Int_<8>()&&
                                                //*max_element_(begin_(fsm_(m_var4)),end_(fsm_(m_var4)),Predicate_<std::less<int> >()) == Int_<9>()&&
                                                //adjacent_find_(begin_(fsm_(m_var4)),end_(fsm_(m_var4))) == end_(fsm_(m_var4))&&
                                                //*find_end_(begin_(fsm_(m_var8)),end_(fsm_(m_var8)),begin_(fsm_(m_var9)),end_(fsm_(m_var9))) 
                                                //    == Int_<1>()&&
                                                //*find_first_of_(begin_(fsm_(m_var8)),end_(fsm_(m_var8)),begin_(fsm_(m_var9)),end_(fsm_(m_var9))) 
                                                //    == Int_<1>()&&
                                                //equal_(begin_(fsm_(m_var9)),end_(fsm_(m_var9)),begin_(fsm_(m_var8)))&&
                                                //*search_(begin_(fsm_(m_var8)),end_(fsm_(m_var8)),begin_(fsm_(m_var9)),end_(fsm_(m_var9))) 
                                                //    == Int_<1>()&&
                                                //includes_(begin_(fsm_(m_var8)),end_(fsm_(m_var8)),begin_(fsm_(m_var9)),end_(fsm_(m_var9)))&&
                                                //!lexicographical_compare_(begin_(fsm_(m_var8)),end_(fsm_(m_var8)),
                                                //                          begin_(fsm_(m_var9)),end_(fsm_(m_var9)))&&
                                                //first_(mismatch_(begin_(fsm_(m_var9)),end_(fsm_(m_var9)),begin_(fsm_(m_var8))))
                                                //    == end_(fsm_(m_var9)) &&
                                                accumulate_(begin_(fsm_(m_var9)),end_(fsm_(m_var9)),Int_<1>(),
                                                            Predicate_<std::plus<int> >()) == Int_<1>()
                                                ),
                                                (process2_(OneSong,*(state_(m_src_it)++))/*,foobar_(fsm_,Int_<0>())*/ ) ),
                                      no_action, 
                                      attributes_ << m_src_it
                                      , configure_<< SomeFlag ),Foreach)


    // replaces the old transition table
    BOOST_MSM_EUML_TRANSITION_TABLE((
          StringFind == Foreach     + OneSong[if_then_else_(true_,true_,true_)],
          Insert     == StringFind  + Found  / (if_then_(true_,no_action)),
          Foreach    == StringFind  + NotFound ,
          Foreach    == Insert      + Done 
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
    BOOST_MSM_EUML_DECLARE_STATE_MACHINE(( transition_table, //STT
                                        init_ << Foreach, // Init
                                        //insert_(state_(m_var4),begin_(state_(m_var2)),end_(state_(m_var2))),
                                        (insert_(state_(m_var4),Int_<5>()),insert_(state_(m_var4),Int_<6>()),insert_(state_(m_var4),Int_<7>()),
                                         insert_(state_(m_var4),Int_<8>()),insert_(state_(m_var4),Int_<9>()),
                                        associative_erase_(state_(m_var4),Int_<6>()),associative_erase_(state_(m_var4),begin_(state_(m_var4))),
                                        associative_erase_(state_(m_var4),begin_(state_(m_var4)),++begin_(state_(m_var4))),
                                        insert_(state_(m_var2),begin_(state_(m_var2)),begin_(state_(m_var3)),end_(state_(m_var3))),
                                        state_(m_var5)[Int_<0>()]=Int_<0>(),state_(m_var5)[Int_<1>()]=Int_<1>()
                                        ,attribute_(substate_(Foreach,fsm_),m_src_it)
                                            = begin_(fsm_(m_src_container))
                                        //,fill_(begin_(state_(m_var9)),end_(state_(m_var9)),Int_<0>())
                                        //,fill_n_(begin_(state_(m_var9)),Size_t_<2>(),Int_<0>())
                                        //,transform_(begin_(state_(m_var4)),end_(state_(m_var4)),begin_(state_(m_var2)),begin_(state_(m_var4)),
                                        //            Predicate_<std::plus<int> >())
                                        //,process_(Done,fsm_(),fsm_)
                                        //,process_(Done,fsm_)
                                        //,fsm_
                                        //,foobar_(state_,Int_<0>(),Int_<1>())
                                        //,nth_element_(begin_(state_(m_var9)),++begin_(state_(m_var9)),end_(state_(m_var9)),Predicate_<std::less<int> >())
                                        //,partial_sort_(begin_(state_(m_var9)),end_(state_(m_var9)),end_(state_(m_var9)),Predicate_<std::less<int> >())
                                        //,partial_sort_copy_(begin_(state_(m_var9)),end_(state_(m_var9)),begin_(state_(m_var9)),end_(state_(m_var9)),Predicate_<std::less<int> >())
                                        //,list_sort_(state_(m_var2))
                                        //,sort_(begin_(state_(m_var9)),end_(state_(m_var9)),Predicate_<std::less<int> >())
                                        //,inner_product_(begin_(state_(m_var9)),end_(state_(m_var9)),begin_(state_(m_var9)),Int_<1>())
                                        //,replace_copy_(begin_(state_(m_var4)),end_(state_(m_var4)),begin_(state_(m_var4)),Int_<8>(),Int_<7>())
                                        //,replace_copy_if_(begin_(state_(m_var4)),end_(state_(m_var4)),begin_(state_(m_var4)),Predicate_<LookFor<9> >(),Int_<8>())
                                        //,replace_(begin_(state_(m_var4)),end_(state_(m_var4)),Int_<8>(),Int_<7>())
                                        //,replace_if_(begin_(state_(m_var4)),end_(state_(m_var4)),Predicate_<LookFor<9> >(),Int_<8>())
                                        //,adjacent_difference_(begin_(state_(m_var9)),end_(state_(m_var9)),begin_(state_(m_var9)))
                                        //,partial_sum_(begin_(state_(m_var9)),end_(state_(m_var9)),begin_(state_(m_var9)))
                                        //,inner_product_(begin_(state_(m_var9)),end_(state_(m_var9)),begin_(state_(m_var9)),Int_<1>())
                                        //,next_permutation_(begin_(state_(m_var9)),end_(state_(m_var9)),Predicate_<std::less<int> >())
                                        //,prev_permutation_(begin_(state_(m_var9)),end_(state_(m_var9)),Predicate_<std::less<int> >())
                                        //,set_union_(begin_(state_(m_var9)),end_(state_(m_var9)),begin_(state_(m_var9)),end_(state_(m_var9)),begin_(state_(m_var9)))
                                        //,inplace_merge_(begin_(state_(m_var9)),end_(state_(m_var9)),end_(state_(m_var9)),Predicate_<std::less<int> >())
                                        //,merge_(begin_(state_(m_var9)),end_(state_(m_var9)),begin_(state_(m_var9)),end_(state_(m_var9))
                                        //        ,begin_(state_(m_var9)),Predicate_<std::less<int> >())
                                        //,stable_sort_(begin_(state_(m_var9)),end_(state_(m_var9)),Predicate_<std::less<int> >())
                                        //,partition_(begin_(state_(m_var2)),end_(state_(m_var2)),Predicate_<LessThan<3> >())
                                        //,stable_partition_(begin_(state_(m_var2)),end_(state_(m_var2)),Predicate_<LessThan<3> >())
                                        //,rotate_copy_(begin_(state_(m_var2)),++begin_(state_(m_var2)),end_(state_(m_var2)),begin_(state_(m_var2)))
                                        //,rotate_(begin_(state_(m_var2)),++begin_(state_(m_var2)),end_(state_(m_var2)))
                                        //,unique_(begin_(state_(m_var2)),end_(state_(m_var2)))
                                        //,unique_copy_(begin_(state_(m_var2)),end_(state_(m_var2)),begin_(state_(m_var2)))
                                        //,random_shuffle_(begin_(state_(m_var9)),end_(state_(m_var9)))
                                        //,generate_(begin_(state_(m_var9)),end_(state_(m_var9)),Predicate_<Generator>())
                                        //,generate_n_(begin_(state_(m_var9)),Int_<2>(),Predicate_<Generator>())
                                        //,reverse_copy_(begin_(state_(m_var2)),end_(state_(m_var2)),begin_(state_(m_var2)))
                                        //erase_(state_(m_src_container),
                                        //       remove_if_(begin_(state_(m_src_container)),end_(state_(m_src_container)),
                                        //                  Predicate_<RemoveDummy>()),
                                        //       end_(state_(m_src_container))),
                                        //list_remove_(state_(m_var2),Int_<3>()),
                                        //remove_copy_if_(begin_(state_(m_var9)),end_(state_(m_var9)),back_inserter_(state_(m_var2)), 
                                        //                Predicate_<LookFor<2> >() )
                                        //for_each_(begin_(state_(m_src_container)),end_(state_m_src_container()),
                                        //          Predicate_<Print>() ),
                                        //copy_(begin_(state_(m_var9)),end_(state_(m_var9)),inserter_(state_(m_var2),end_(state_(m_var2)))),
                                        //reverse_(begin_(state_(m_var2)),end_(state_(m_var2)))
                                        ),
                                        //no_action, // Entry
                                        //splice_(state_(m_var2),begin_(state_(m_var2)),state_(m_var3),begin_(state_(m_var3)),end_(state_(m_var3))),
                                        //(list_remove_(state_(m_var2),Int_<3>()),list_merge_(state_(m_var2),state_(m_var3),Comparator())),//no_action, // Entry
                                        no_action, // Exit
                                        attributes_ << m_src_container // song list
                                                    << m_tgt_container // result
                                                    << m_var2
                                                    << m_var3
                                                    << m_var4
                                                    << m_var5
                                                    << m_var6
                                                    << m_var7
                                                    << m_var8
                                                    << m_var9
                                                    << m_var10,
                                        configure_<< no_configure_,
                                        Log_No_Transition
                                    ),iPodSearch_helper)

    struct iPodSearch_ : public iPodSearch_helper, public some_base 
    {
    };


    // choice of back-end
    typedef msm::back::state_machine<iPodSearch_> iPodSearch;

    void test()
    {        
        iPodSearch search;
        // fill our song list
        //search.get_attribute<m_src_container>().push_back(OneSong("She-Dummy. Remove this one"));
        search.get_attribute(m_src_container).push_back(OneSong_impl("Let it be"));
        search.get_attribute(m_src_container).push_back(OneSong_impl("Yellow submarine"));
        search.get_attribute(m_src_container).push_back(OneSong_impl("Twist and Shout"));
        search.get_attribute(m_src_container).push_back(OneSong_impl("She Loves You"));

        search.get_attribute(m_var2).push_back(1);
        search.get_attribute(m_var2).push_back(3);
        search.get_attribute(m_var2).push_back(4);

        search.get_attribute(m_var3).push_back(2);
        search.get_attribute(m_var3).push_back(4);

        search.get_attribute(m_var6) = "S";
        search.get_attribute(m_var7) = "- Some text";

        search.get_attribute(m_var8).push_back(1);
        search.get_attribute(m_var8).push_back(2);
        search.get_attribute(m_var8).push_back(3);
        search.get_attribute(m_var8).push_back(4);

        search.get_attribute(m_var9).push_back(1);
        search.get_attribute(m_var9).push_back(2);


        // look for "She Loves You" using the first letters
        // BOOST_MSM_EUML_STATE_NAME returns the name of the event type of which StringFind is an instance 
        search.get_state<BOOST_MSM_EUML_STATE_NAME(StringFind)&>().get_attribute(m_letters)="Sh";
        // needed to start the highest-level SM. This will call on_entry and mark the start of the SM
        search.start(); 
        // display all the songs
        for (list<OneSong_impl>::const_iterator it = search.get_attribute(m_tgt_container).begin(); 
             it != search.get_attribute(m_tgt_container).end();++it)
        {
            cout << "candidate song:" << (*it).get_data() << endl;
        }
        for (list<int>::const_iterator iti = search.get_attribute(m_var2).begin(); 
            iti != search.get_attribute(m_var2).end();++iti)
        {
            cout << "int in attribute m_var2:" << (*iti) << endl;
        }
        for (set<int>::const_iterator its = search.get_attribute(m_var4).begin(); 
            its != search.get_attribute(m_var4).end();++its)
        {
            cout << "int in attribute m_var4:" << (*its) << endl;
        }
        cout << "search using more letters" << endl;
        // look for "She Loves You" using more letters
        search.get_state<BOOST_MSM_EUML_STATE_NAME(StringFind)&>().get_attribute(m_letters)="She";
        search.get_attribute(m_tgt_container).clear();
        search.start(); 
        // display all the songs
        for (list<OneSong_impl>::const_iterator it = search.get_attribute(m_tgt_container).begin(); 
             it != search.get_attribute(m_tgt_container).end();++it)
        {
            cout << "candidate song:" << (*it).get_data() << endl;
        }
    }
}

int main()
{
    test();
    return 0;
}
