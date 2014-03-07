// Copyright Vladimir Prus 2004.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "canvas.h"

#include <qlabel.h>
#include <qcanvas.h>
#include <qlayout.h>

Canvas::Canvas(QWidget* parent)
: QWidget(parent)
{
    m_pen = QPen(QColor(255, 128, 128));
    m_brushes = new QBrush[2];
    m_brushes[0] = QBrush(QColor(255, 0, 0));
    m_brushes[1] = QBrush(QColor(0, 255, 0));
    m_current_brush = 0;
    
    m_canvas = new QCanvas(this);
    m_canvas->resize(4*1600, 600);
    
    redraw();   
    
    QVBoxLayout* l = new QVBoxLayout(this); 
    
    m_canvas_view = new QCanvasView(m_canvas, this);
    l->addWidget(m_canvas_view);    
    m_canvas_view->resize(rect().size());
    m_canvas_view->show();    
}

Canvas::~Canvas()
{
    delete m_brushes;
}

void Canvas::redraw()
{
    QCanvasItemList l = m_canvas->allItems();    
    for(QCanvasItemList::iterator i = l.begin(),
            e = l.end(); i != e; ++i)
    {
        delete *i;
    }
    
    unsigned count = 0;
    for (unsigned x = 10; x < 4*1600; x += 20)
        for (unsigned y = 10; y < 600; y += 20) {
            QCanvasRectangle* r = new QCanvasRectangle(x, y, 10, 10, m_canvas);
            r->setPen(m_pen);
            r->setBrush(m_brushes[m_current_brush]);
            r->show();  
            ++count;  
            QCanvasText* t = new QCanvasText("D", m_canvas);
            t->move(x, y);
            t->show();
            ++count;
        }
    
    (new QCanvasText(QString::number(count), m_canvas))->show();
    m_canvas->setAllChanged();

}

void Canvas::change_color()
{
    m_current_brush = (m_current_brush + 1)%2;
    redraw();
    m_canvas->update();
}

