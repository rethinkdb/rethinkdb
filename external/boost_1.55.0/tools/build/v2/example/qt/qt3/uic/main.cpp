// Copyright Felix E. Klee, 2003
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "hello_world_widget.h"
#include <qapplication.h>

#include <qpushbutton.h>

int main(int argc, char **argv) {
    QApplication a(argc, argv);
    HelloWorldWidget w;
    QObject::connect(static_cast<QObject*>(w.OkButton), SIGNAL(clicked()), &w, SLOT(close()));
    a.setMainWidget(&w);
    w.show();
    return a.exec();
}
