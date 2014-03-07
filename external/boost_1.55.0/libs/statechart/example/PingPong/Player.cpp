//////////////////////////////////////////////////////////////////////////////
// Copyright 2008 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include "Player.hpp"
#include "Waiting.hpp" // Waiting.hpp is only included here



//////////////////////////////////////////////////////////////////////////////
void Player::initiate_impl()
{
  // Since we can only initiate at a point where the definitions of all the
  // states in the initial state configuration are known, we duplicate
  // the implementation of asynchronous_state_machine<>::initiate_impl() here
  sc::state_machine< Player, Waiting, MyAllocator >::initiate();
}


unsigned int Player::totalNoOfProcessedEvents_ = 0;
