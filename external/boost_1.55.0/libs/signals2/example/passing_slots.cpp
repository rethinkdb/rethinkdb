// Example program showing passing of slots through an interface.
//
// Copyright Douglas Gregor 2001-2004.
// Copyright Frank Mori Hess 2009.
//
// Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// For more information, see http://www.boost.org

#include <iostream>
#include <boost/signals2/signal.hpp>

//[ passing_slots_defs_code_snippet
// a pretend GUI button
class Button
{
  typedef boost::signals2::signal<void (int x, int y)> OnClick;
public:
  typedef OnClick::slot_type OnClickSlotType;
  // forward slots through Button interface to its private signal
  boost::signals2::connection doOnClick(const OnClickSlotType & slot);

  // simulate user clicking on GUI button at coordinates 52, 38
  void simulateClick();
private:
  OnClick onClick;
};

boost::signals2::connection Button::doOnClick(const OnClickSlotType & slot)
{
  return onClick.connect(slot);
}

void Button::simulateClick()
{
  onClick(52, 38);
}

void printCoordinates(long x, long y)
{
  std::cout << "(" << x << ", " << y << ")\n";
}
//]

int main()
{
//[ passing_slots_usage_code_snippet
  Button button;
  button.doOnClick(&printCoordinates);
  button.simulateClick();
//]
  return 0;
}
