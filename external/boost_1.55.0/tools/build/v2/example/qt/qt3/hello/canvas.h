// Copyright Vladimir Prus 2004.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)


#ifndef CANVAS_VP_2004_08_31
#define CANVAS_VP_2004_08_31

#include <qmainwindow.h>
#include <qpen.h>
#include <qbrush.h>

class Canvas : public QWidget
{
    Q_OBJECT
public:
    Canvas(QWidget* parent);

    virtual ~Canvas();
    
public slots:
    void change_color();    
    
private:
    void redraw();
    class QCanvas* m_canvas;
    class QCanvasView* m_canvas_view;
    class QPen m_pen;
    class QBrush* m_brushes;    
    int m_current_brush;
};

#endif

