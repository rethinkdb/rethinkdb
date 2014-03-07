// Copyright 2010 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef FSM_AS_PTR_H
#define FSM_AS_PTR_H

#include <boost/shared_ptr.hpp>

class player
{
public:
    player();
    virtual ~player(){}

    // public interface
    void do_play();
    void do_pause();
    void do_open_close();
    void do_end_pause();
    void do_stop();
    void do_cd_detected();

private:
    // my state machine, hidden
    boost::shared_ptr<void>   fsm_;

};

#endif //FSM_AS_PTR_H
