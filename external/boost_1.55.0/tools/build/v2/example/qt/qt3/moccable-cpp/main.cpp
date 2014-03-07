// Copyright Vladimir Prus 2005.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)


#include <qwidget.h>
#include <qpushbutton.h>
#include <qapplication.h>

#include <iostream>

class My_widget : public QWidget
{
    Q_OBJECT
public:
    My_widget() : QWidget() 
    {
        QPushButton* b = new QPushButton("Push me", this);
    
        connect(b, SIGNAL(clicked()), this, SLOT(theSlot()));
    }

private slots:
    void theSlot()
    {
        std::cout << "Clicked\n";
    }    
    
};

int main(int ac, char* av[])
{
    QApplication app(ac, av);
    My_widget mw;
    mw.show();
    app.setMainWidget(&mw);
    app.exec();
}

#include "main.moc"
