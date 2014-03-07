/*-----------------------------------------------------------------------------+
Copyright (c) 2007-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
Copyright (c) 1999-2006: Cortex Software GmbH, Kantstrasse 57, Berlin
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------+
splititvmap_shell.cpp provides  a simple test shell for splitting interval maps.
The shell also gives you a good idea how interval container are working.
+-----------------------------------------------------------------------------*/
#include <iostream>

#include <boost/icl/split_interval_set.hpp>
#include <boost/icl/split_interval_map.hpp>
#include <boost/icl/interval_map.hpp>

using namespace std;
using namespace boost;
using namespace boost::icl;

void instructions()
{
    cout << "+++++ Test shell for split interval map +++++\n";
    cout << "Type: q e or 0  to quit\n";
    cout << "Type: +         for insertions\n";
    cout << "Type: -         for subtraction of ([a,b],value)\n";
    cout << "Type: _         for subtraction of [a,b]\n";
    cout << "Type: j         to join contiguous intervals\n";
    cout << "Type: s         to compute total size\n";
}

void wrongInput()
{
    cout << "Wrong Input ------------------\n";
    instructions();
}


template <class MapTV>
void mapTestShell()
{
    MapTV m1;

    try {
        char cmd = 'b';
        typename MapTV::domain_type 
            lwb = typename MapTV::domain_type(), 
            upb = typename MapTV::domain_type();
            
        typename MapTV::codomain_type 
            val = typename MapTV::codomain_type();

        instructions();

        for(;;)
        {
            cout << "> ";
            cin >> cmd ;

            switch(cmd) 
            {
            case 'q':
            case 'e':
            case '0': cout << "good bye\n"; return;
            case '+': 
                {
                    cout << "input: lwb upb val >> ";
                    cin >> lwb >> upb >> val;
                    typename MapTV::interval_type 
                        itv = typename MapTV::interval_type(lwb,upb);
                    m1 += make_pair(itv,val);

                    cout << "+" << itv << " " << val << " =" << endl;
                    cout << "{" << m1 << "}" << endl;

                }
                break;
            case '-': 
                {
                    cout << "input: lwb upb val >> ";
                    cin >> lwb >> upb >> val;
                    typename MapTV::interval_type 
                        itv = typename MapTV::interval_type(lwb,upb);
                    m1 -= make_pair(itv,val);

                    cout << "-" << itv << " " << val << " =" << endl;
                    cout << "{" << m1 << "}" << endl;

                }
                break;
            case 'j':
                {
                    icl::join(m1);
                    cout << "{" << m1 << "}" << endl;
                }
                break;
            case 's':
                {
                    cout << "size = " << m1.size() << endl;
                }
                break;

            default: wrongInput();
            }
        } // end while
    }
    catch (exception& e)
    {
        cout << "splititvmap_shell: exception caught: " << endl
             << e.what() << endl;
    }    
    catch (...)
    {
        cout << "splititvmap_shell: unknown exception caught" << endl;
    }    
}


int main()
{
    cout << ">>Interval Container Library: Test splititvmap_shell.cpp <<\n";
    cout << "-----------------------------------------------------------\n";
    mapTestShell< interval_map<int, int> >();

    return 0;
}

