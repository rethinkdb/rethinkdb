// Copyright Vladimir Prus 2005.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <QtGui/qapplication.h>

#include <QtGui/qwidget.h>
#include <QtGui/qpainter.h>
#include <QtGui/qpainterpath.h>

#include <stdlib.h>
#include <math.h>

class Arrow_widget : public QWidget
{
    Q_OBJECT
public:
    Arrow_widget(QWidget* parent = 0);

public slots:
    void slotChangeColor();

private:
    void draw_arrow(int x1, int y1, int x2, int y2, QPainter& painter);
    void paintEvent(QPaintEvent*);

private:
    int color_;
};
