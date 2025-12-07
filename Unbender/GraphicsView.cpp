#include "GraphicsView.h"
#include "MarkerItem.h"

#include <QWheelEvent>
#include <QGraphicsItem.h>

GraphicsView::GraphicsView(QGraphicsScene *scene, QWidget *parent) : QGraphicsView(scene, parent)
{
    // setDragMode(QGraphicsView::ScrollHandDrag);
    // setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
}

void GraphicsView::wheelEvent(QWheelEvent *event)
{
    const double scaleFactor = 1.15; // zoom in/out factor

    if (event->angleDelta().y() > 0)
    {
        // Zoom in
        scale(scaleFactor, scaleFactor);
    }
    else
    {
        // Zoom out
        scale(1.0 / scaleFactor, 1.0 / scaleFactor);
    }
}

void GraphicsView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        QPointF center = mapToScene(event->pos());
        if (!m_cursor)
        {
            MarkerItem::MarkerShape shape = MarkerItem::Cross;
            qreal size = 10.0;
            QColor color = Qt::red;
            m_cursor = new MarkerItem(center, shape, size, color);
            scene()->addItem(m_cursor);
        }
   }
}

void GraphicsView::mouseMoveEvent(QMouseEvent *event)
{
}

void GraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
    }
}
