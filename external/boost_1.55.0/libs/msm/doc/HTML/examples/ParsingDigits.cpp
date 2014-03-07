// Copyright 2010 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#define FUSION_MAX_VECTOR_SIZE 20

#include <boost/msm/back/state_machine.hpp>
#include "char_event_dispatcher.hpp"
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/timer.hpp>

namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::front;

#include <iostream>
#ifdef WIN32
#include "windows.h"
#else
#include <sys/time.h>
#endif

// events
struct end_sub {template <class Event> end_sub(Event const&){}};
struct other_char {};
struct default_char {};
struct eos {};


namespace test_fsm // Concrete FSM implementation
{
    // Concrete FSM implementation 
    struct parsing_ : public msm::front::state_machine_def<parsing_>
    {
        // no need for exception handling or message queue
        typedef int no_exception_thrown;
        typedef int no_message_queue;

        struct Waiting : public msm::front::state<>  
        {
            // optional entry/exit methods
            //template <class Event,class FSM>
            //void on_entry(Event const&,FSM& ) {std::cout << "entering: Waiting" << std::endl;}
            //template <class Event,class FSM>
            //void on_exit(Event const&,FSM& ) {std::cout << "leaving: Waiting" << std::endl;}
        };
        struct Digit1 : public msm::front::state<>  
        {
            // optional entry/exit methods
            //template <class Event,class FSM>
            //void on_entry(Event const&,FSM& ) {std::cout << "entering: Digit1" << std::endl;}
            //template <class Event,class FSM>
            //void on_exit(Event const&,FSM& ) {std::cout << "leaving: Digit1" << std::endl;}
        };
        struct Digit2 : public msm::front::state<>  
        {
            // optional entry/exit methods
            //template <class Event,class FSM>
            //void on_entry(Event const&,FSM& ) {std::cout << "entering: Digit2" << std::endl;}
            //template <class Event,class FSM>
            //void on_exit(Event const&,FSM& ) {std::cout << "leaving: Digit2" << std::endl;}
        };
        struct Digit3 : public msm::front::state<>  
        {
            // optional entry/exit methods
            //template <class Event,class FSM>
            //void on_entry(Event const&,FSM& ) {std::cout << "entering: Digit3" << std::endl;}
            //template <class Event,class FSM>
            //void on_exit(Event const&,FSM& ) {std::cout << "leaving: Digit3" << std::endl;}
        };
        struct Digit4 : public msm::front::state<>  
        {
            // optional entry/exit methods
            //template <class Event,class FSM>
            //void on_entry(Event const&,FSM& ) {std::cout << "entering: Digit4" << std::endl;}
            //template <class Event,class FSM>
            //void on_exit(Event const&,FSM& ) {std::cout << "leaving: Digit4" << std::endl;}
        };
        struct MinusChar1 : public msm::front::state<>  
        {
            // optional entry/exit methods
            //template <class Event,class FSM>
            //void on_entry(Event const&,FSM& ) {std::cout << "entering: MinusChar1" << std::endl;}
            //template <class Event,class FSM>
            //void on_exit(Event const&,FSM& ) {std::cout << "leaving: MinusChar1" << std::endl;}
        };
        struct Digit5 : public msm::front::state<>  
        {
            // optional entry/exit methods
            //template <class Event,class FSM>
            //void on_entry(Event const&,FSM& ) {std::cout << "entering: Digit5" << std::endl;}
            //template <class Event,class FSM>
            //void on_exit(Event const&,FSM& ) {std::cout << "leaving: Digit5" << std::endl;}
        };
        struct Digit6 : public msm::front::state<>  
        {
            // optional entry/exit methods
            //template <class Event,class FSM>
            //void on_entry(Event const&,FSM& ) {std::cout << "entering: Digit6" << std::endl;}
            //template <class Event,class FSM>
            //void on_exit(Event const&,FSM& ) {std::cout << "leaving: Digit6" << std::endl;}
        };
        struct Digit7 : public msm::front::state<>  
        {
            // optional entry/exit methods
            //template <class Event,class FSM>
            //void on_entry(Event const&,FSM& ) {std::cout << "entering: Digit7" << std::endl;}
            //template <class Event,class FSM>
            //void on_exit(Event const&,FSM& ) {std::cout << "leaving: Digit7" << std::endl;}
        };
        struct Digit8 : public msm::front::state<>  
        {
            // optional entry/exit methods
            //template <class Event,class FSM>
            //void on_entry(Event const&,FSM& ) {std::cout << "entering: Digit8" << std::endl;}
            //template <class Event,class FSM>
            //void on_exit(Event const&,FSM& ) {std::cout << "leaving: Digit8" << std::endl;}
        };
        struct MinusChar2 : public msm::front::state<>  
        {
            // optional entry/exit methods
            //template <class Event,class FSM>
            //void on_entry(Event const&,FSM& ) {std::cout << "entering: MinusChar2" << std::endl;}
            //template <class Event,class FSM>
            //void on_exit(Event const&,FSM& ) {std::cout << "leaving: MinusChar2" << std::endl;}
        };
        struct Digit9 : public msm::front::state<>  
        {
            // optional entry/exit methods
            //template <class Event,class FSM>
            //void on_entry(Event const&,FSM& ) {std::cout << "entering: Digit9" << std::endl;}
            //template <class Event,class FSM>
            //void on_exit(Event const&,FSM& ) {std::cout << "leaving: Digit9" << std::endl;}
        };
        struct Digit10 : public msm::front::state<>  
        {
            // optional entry/exit methods
            //template <class Event,class FSM>
            //void on_entry(Event const&,FSM& ) {std::cout << "entering: Digit10" << std::endl;}
            //template <class Event,class FSM>
            //void on_exit(Event const&,FSM& ) {std::cout << "leaving: Digit10" << std::endl;}
        };
        struct Digit11 : public msm::front::state<>  
        {
            // optional entry/exit methods
            //template <class Event,class FSM>
            //void on_entry(Event const&,FSM& ) {std::cout << "entering: Digit11" << std::endl;}
            //template <class Event,class FSM>
            //void on_exit(Event const&,FSM& ) {std::cout << "leaving: Digit11" << std::endl;}
        };
        struct Digit12 : public msm::front::state<>  
        {
            // optional entry/exit methods
            //template <class Event,class FSM>
            //void on_entry(Event const&,FSM& ) {std::cout << "entering: Digit12" << std::endl;}
            //template <class Event,class FSM>
            //void on_exit(Event const&,FSM& ) {std::cout << "leaving: Digit12" << std::endl;}
        };
        struct MinusChar3 : public msm::front::state<>  
        {
            // optional entry/exit methods
            //template <class Event,class FSM>
            //void on_entry(Event const&,FSM& ) {std::cout << "entering: MinusChar3" << std::endl;}
            //template <class Event,class FSM>
            //void on_exit(Event const&,FSM& ) {std::cout << "leaving: MinusChar3" << std::endl;}
        };
        struct Digit13 : public msm::front::state<>  
        {
            // optional entry/exit methods
            //template <class Event,class FSM>
            //void on_entry(Event const&,FSM& ) {std::cout << "entering: Digit13" << std::endl;}
            //template <class Event,class FSM>
            //void on_exit(Event const&,FSM& ) {std::cout << "leaving: Digit13" << std::endl;}
        };
        struct Digit14 : public msm::front::state<>  
        {
            // optional entry/exit methods
            //template <class Event,class FSM>
            //void on_entry(Event const&,FSM& ) {std::cout << "entering: Digit14" << std::endl;}
            //template <class Event,class FSM>
            //void on_exit(Event const&,FSM& ) {std::cout << "leaving: Digit14" << std::endl;}
        };
        struct Digit15 : public msm::front::state<>  
        {
            // optional entry/exit methods
            //template <class Event,class FSM>
            //void on_entry(Event const&,FSM& ) {std::cout << "entering: Digit15" << std::endl;}
            //template <class Event,class FSM>
            //void on_exit(Event const&,FSM& ) {std::cout << "leaving: Digit15" << std::endl;}
        };
        //struct Start : public msm::front::state<> {};
        struct Parsed : public msm::front::state<> {};
        //struct Failed : public msm::front::state<> {};

        // the initial state of the player SM. Must be defined
        typedef Waiting initial_state;
        // transition actions
        struct test_fct
        {
            template <class FSM,class EVT,class SourceState,class TargetState>
            void operator()(EVT const& ,FSM&,SourceState& ,TargetState& )
            {
                std::cout << "Parsed!" << std::endl;
            }
        };

        // guard conditions


        // Transition table for parsing_
        struct transition_table : mpl::vector<
            //    Start         Event                Next      Action                Guard
            //    +-------------+-------------------+---------+---------------------+----------------------+
              Row < Waiting     , digit             , Digit1                                                >,
              Row < Digit1      , digit             , Digit2                                                >,
              Row < Digit2      , digit             , Digit3                                                >,
              Row < Digit3      , digit             , Digit4                                                >,
              Row < Digit4      , event_char<'-'>   , MinusChar1                                            >,
              Row < MinusChar1  , digit             , Digit5                                                >,
              Row < Digit5      , digit             , Digit6                                                >,
              Row < Digit6      , digit             , Digit7                                                >,
              Row < Digit7      , digit             , Digit8                                                >,
              Row < Digit8      , event_char<'-'>   , MinusChar2                                            >,
              Row < MinusChar2  , digit             , Digit9                                                >,
              Row < Digit9      , digit             , Digit10                                               >,
              Row < Digit10     , digit             , Digit11                                               >,
              Row < Digit11     , digit             , Digit12                                               >,
              Row < Digit12     , event_char<'-'>   , MinusChar3                                            >,
              Row < MinusChar3  , digit             , Digit13                                               >,
              Row < Digit13     , digit             , Digit14                                               >,
              Row < Digit14     , digit             , Digit15                                               >,
              Row < Digit15     , eos               , Parsed                                                >,
              Row < Parsed      , eos               , Waiting                                               >
            //    +---------+-------------+---------+---------------------+----------------------+
        > {};


        // Replaces the default no-transition response.
        template <class FSM,class Event>
        void no_transition(Event const& e, FSM&,int state)
        {
            std::cout << "no transition from state " << state
                << " on event " << typeid(e).name() << std::endl;
        }
    };
    typedef msm::back::state_machine<parsing_> parsing;
}

#ifndef WIN32
long mtime(struct timeval& tv1,struct timeval& tv2)
{
    return (tv2.tv_sec-tv1.tv_sec) *1000000 + ((tv2.tv_usec-tv1.tv_usec));
}
#endif

// This declares the statically-initialized char_event_dispatcher instance.
template <class Fsm>
const msm::back::char_event_dispatcher<Fsm>
msm::back::char_event_dispatcher<Fsm>::instance;

struct Parser
{
    Parser():p(){p.start();}
    void new_char(char c)
    {
        typedef msm::back::char_event_dispatcher<test_fsm::parsing> table;
        table::instance.process_event(p,c);
    }
    void finish_string(){p.process_event(eos());}
    void reinit(){p.process_event(eos());}
    test_fsm::parsing p;
};

void msm_match(const char* input)
{
    test_fsm::parsing p;
    p.start();

    int j=0;
    while(input[j])
        //for (size_t j=0;j<len;++j)
    {
        switch (input[j])
        {
        case '0':
            p.process_event(char_0());
            break;
        case '1':
            p.process_event(char_1());
            break;
        case '2':
            p.process_event(char_2());
            break;
        case '3':
            p.process_event(char_3());
            break;
        case '4':
            p.process_event(char_4());
            break;
        case '5':
            p.process_event(char_5());
            break;
        case '6':
            p.process_event(char_6());
            break;
        case '7':
            p.process_event(char_7());
            break;
        case '8':
            p.process_event(char_8());
            break;
        case '9':
            p.process_event(char_9());
            break;
        case '-':
            p.process_event(event_char<'-'>());
            break;
        default:
            p.process_event(default_char());
            break;
        }
        ++j;
    }
    p.process_event(eos());
    p.process_event(eos());
}

double time_match(const char* text)
{
    boost::timer tim;
    int iter = 1;
    int counter, repeats;
    double result = 0;
    double run;
    do
    {
        tim.restart();
        for(counter = 0; counter < iter; ++counter)
        {
            msm_match( text);
        }
        result = tim.elapsed();
        iter *= 2;
    } while(result < 0.5);
    iter /= 2;

    // repeat test and report least value for consistency:
    for(repeats = 0; repeats < 10; ++repeats)
    {
        tim.restart();
        for(counter = 0; counter < iter; ++counter)
        {
            msm_match( text);
        }
        run = tim.elapsed();
        result = (std::min)(run, result);
    }
    return result / iter;
}
int main()
{
    // for timing
#ifdef WIN32
    LARGE_INTEGER res;
    ::QueryPerformanceFrequency(&res);
    LARGE_INTEGER li,li2;
#else
    struct timeval tv1,tv2;
    gettimeofday(&tv1,NULL);
#endif

    test_fsm::parsing p;
    p.start();
    const char* input = "1234-5678-1234-456";
    size_t len = strlen(input);
    // for timing
#ifdef WIN32
    ::QueryPerformanceCounter(&li);
#else
    gettimeofday(&tv1,NULL);
#endif
    for (int i=0;i<1000;++i)
    {
        int j=0;
        while(input[j])
        //for (size_t j=0;j<len;++j)
        {
            switch (input[j])
            {
            case '0':
                p.process_event(char_0());
                break;
            case '1':
                p.process_event(char_1());
                break;
            case '2':
                p.process_event(char_2());
                break;
            case '3':
                p.process_event(char_3());
                break;
            case '4':
                p.process_event(char_4());
                break;
            case '5':
                p.process_event(char_5());
                break;
            case '6':
                p.process_event(char_6());
                break;
            case '7':
                p.process_event(char_7());
                break;
            case '8':
                p.process_event(char_8());
                break;
            case '9':
                p.process_event(char_9());
                break;
            case '-':
                p.process_event(event_char<'-'>());
                break;
            default:
                p.process_event(default_char());
                break;
            }
            ++j;
        }
        p.process_event(eos());
        p.process_event(eos());
    }
#ifdef WIN32
    ::QueryPerformanceCounter(&li2);
#else
    gettimeofday(&tv2,NULL);
#endif
#ifdef WIN32
    std::cout << "msm(1) took in s:" << (double)(li2.QuadPart-li.QuadPart)/res.QuadPart <<"\n" <<std::endl;
#else
    std::cout << "msm(1) took in us:" <<  mtime(tv1,tv2) <<"\n" <<std::endl;
#endif

    Parser parse;
    // for timing
#ifdef WIN32
    ::QueryPerformanceCounter(&li);
#else
    gettimeofday(&tv1,NULL);
#endif
    for (int i=0;i<1000;++i)
    {
        for (size_t j=0;j<len;++j)
        {
            parse.new_char(input[j]);
        }
        parse.finish_string();
        parse.reinit();
    }
#ifdef WIN32
    ::QueryPerformanceCounter(&li2);
#else
    gettimeofday(&tv2,NULL);
#endif
#ifdef WIN32
    std::cout << "msm(2) took in s:" << (double)(li2.QuadPart-li.QuadPart)/res.QuadPart <<"\n" <<std::endl;
#else
    std::cout << "msm(2) took in us:" <<  mtime(tv1,tv2) <<"\n" <<std::endl;
#endif
    std::cout << "msm(3) took in s:" << time_match(input) <<"\n" <<std::endl;
    return 0;
}

