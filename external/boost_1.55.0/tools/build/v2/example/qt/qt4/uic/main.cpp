// Copyright Felix E. Klee, 2003
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "ui_hello_world_widget.h"
#include <qapplication.h>
#include <qwidget.h>

#include <qpushbutton.h>

int main(int argc, char **argv) {
    QApplication a(argc, argv);

    QWidget w;
    Ui::HelloWorldWidget wm;
    wm.setupUi(&w);

    QObject::connect(wm.OkButton, SIGNAL(clicked()), &w, SLOT(close()));

    w.show();
    return a.exec();
}
