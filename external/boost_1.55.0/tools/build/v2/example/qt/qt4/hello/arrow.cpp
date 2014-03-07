// Copyright Vladimir Prus 2005.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "arrow.h"

#include <QtGui/qapplication.h>

#include <QtGui/qwidget.h>
#include <QtGui/qpainter.h>
#include <QtGui/qpainterpath.h>

#include <stdlib.h>
#include <math.h>

Arrow_widget::Arrow_widget(QWidget* parent) : QWidget(parent), color_(0)
{
    QPalette pal = palette();
    pal.setBrush(backgroundRole(), QBrush(Qt::white));
    setPalette(pal);
}

void Arrow_widget::slotChangeColor()
{
    color_ = (color_ + 1) % 3;
    update();
}

void 
Arrow_widget::draw_arrow(int x1, int y1, int x2, int y2, QPainter& painter)
{
    // The length of the from the tip of the arrow to the point
    // where line starts.
    const int arrowhead_length = 16;
    
    QPainterPath arrow;
    arrow.moveTo(x1, y1);
    
    // Determine the angle of the straight line.
    double a1 = (x2-x1);
    double a2 = (y2-y1);
    double b1 = 1;
    double b2 = 0;
    
    double straight_length = sqrt(a1*a1 + a2*a2);
    
    double dot_product = a1*b1 + a2*b2;
    double cosine = dot_product/
        (sqrt(pow(a1, 2) + pow(a2, 2))*sqrt(b1 + b2));
    double angle = acos(cosine);
    if (y1 < y2)
    {
        angle = -angle;
    }
    double straight_angle = angle*180/M_PI;
    
    double limit = 10;
    
    double angle_to_vertical;
    if (fabs(straight_angle) < 90)
        angle_to_vertical = fabs(straight_angle);
    else if (straight_angle > 0)
        angle_to_vertical = 180-straight_angle;
    else
        angle_to_vertical = 180-(-straight_angle);
    
    double angle_delta = 0;
    if (angle_to_vertical > limit)
        angle_delta = 30 * (angle_to_vertical - limit)/90;
    double start_angle = straight_angle > 0 
        ? straight_angle - angle_delta :
        straight_angle + angle_delta;
    
    
    QMatrix m1;
    m1.translate(x1, y1);
    m1.rotate(-start_angle);
    
    double end_angle = straight_angle > 0 
        ? (straight_angle + 180 + angle_delta) :
        (straight_angle + 180 - angle_delta);
    
    QMatrix m2;
    m2.reset();
    m2.translate(x2, y2);        
    m2.rotate(-end_angle);
    
    arrow.cubicTo(m1.map(QPointF(straight_length/2, 0)),              
                  m2.map(QPointF(straight_length/2, 0)),
                  m2.map(QPointF(arrowhead_length, 0)));
    
    painter.save();
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(arrow);    
    painter.restore();
    
    painter.save();
    painter.translate(x2, y2);
    
    painter.rotate(-90);
    painter.rotate(-end_angle);
    painter.rotate(180);
    
    QPolygon arrowhead(4);
    arrowhead.setPoint(0, 0, 0);
    arrowhead.setPoint(1, arrowhead_length/3, -arrowhead_length*5/4);
    arrowhead.setPoint(2, 0, -arrowhead_length);
    arrowhead.setPoint(3, -arrowhead_length/3, -arrowhead_length*5/4);
    
    painter.drawPolygon(arrowhead);
    
    painter.restore();            
    
}


void Arrow_widget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    
    p.setRenderHint(QPainter::Antialiasing);
    
    int base_x = 550;
    int base_y = 200;

    if (color_ == 0)   
        p.setBrush(Qt::black);
    else if (color_ == 1)
        p.setBrush(Qt::green);
    else if (color_ == 2)
        p.setBrush(Qt::yellow);
    else
        p.setBrush(Qt::black);
    
    for (int x_step = 0; x_step < 6; ++x_step)
    {
        for (int y_step = 1; y_step <= 3; ++y_step)
        {
            draw_arrow(base_x, base_y, base_x+x_step*100, 
                       base_y - y_step*50, p);
            
            draw_arrow(base_x, base_y, base_x+x_step*100, 
                       base_y + y_step*50, p);
            
            draw_arrow(base_x, base_y, base_x-x_step*100, 
                       base_y + y_step*50, p);
            
            draw_arrow(base_x, base_y, base_x-x_step*100, 
                       base_y - y_step*50, p);
        }
    }

    draw_arrow(50, 400, 1000, 450, p);
    draw_arrow(1000, 400, 50, 450, p);
    
}

