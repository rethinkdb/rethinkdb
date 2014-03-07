/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga  2006-2013
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////
//[doc_window_code
#include <boost/intrusive/list.hpp>

using namespace boost::intrusive;

//An abstract class that can be inserted in an intrusive list
class Window : public list_base_hook<>
{
   public:
   //This is a container those value is an abstract class: you can't do this with std::list.
   typedef list<Window> win_list;

   //A static intrusive list declaration
   static win_list all_windows;

   //Constructor. Includes this window in the list
   Window()             {  all_windows.push_back(*this);  }
   //Destructor. Removes this node from the list
   virtual ~Window()    {  all_windows.erase(win_list::s_iterator_to(*this));  }
   //Pure virtual function to be implemented by derived classes
   virtual void Paint() = 0;
};

//The static intrusive list declaration
Window::win_list Window::all_windows;

//Some Window derived classes
class FrameWindow :  public Window
{  void Paint(){/**/} };

class EditWindow :  public Window
{  void Paint(){/**/} };

class CanvasWindow :  public Window
{  void Paint(){/**/} };

//A function that prints all windows stored in the intrusive list
void paint_all_windows()
{
   for(Window::win_list::iterator i(Window::all_windows.begin())
                                , e(Window::all_windows.end())
      ; i != e; ++i)
      i->Paint();
}

//...

//A class derived from Window
class MainWindow  :  public Window
{
   FrameWindow   frame_;  //these are derived from Window too
   EditWindow    edit_;
   CanvasWindow  canvas_;

   public:
   void Paint(){/**/}
   //...
};

//Main function
int main()
{
   //When a Window class is created, is automatically registered in the global list
   MainWindow window;

   //Paint all the windows, sub-windows and so on
   paint_all_windows();

   //All the windows are automatically unregistered in their destructors.
   return 0;
}
//]
