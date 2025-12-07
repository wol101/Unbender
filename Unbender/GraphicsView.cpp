#include "GraphicsView.h"
#include "MarkerItem.h"

#include <QWheelEvent>
#include <QGraphicsItem>
#include <QMainWindow>
#include <QStatusBar>

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
        if (this->items().size() > 0)
        {
            QPointF centre = mapToScene(event->pos());
            if (!m_cursor)
            {
                MarkerItem::MarkerShape shape = MarkerItem::Cross;
                qreal size = 10.0;
                QColor colour = Qt::red;
                m_cursor = new MarkerItem(centre, shape, size, colour);
                scene()->addItem(m_cursor);
            }
            else
            {
                m_cursor->setPos(centre);
            }
            QMainWindow* mainWin = qobject_cast<QMainWindow*>(this->window());
            if (mainWin) { mainWin->statusBar()->showMessage(QString("Cursor x = %1 y = %2").arg(m_cursor->pos().x()).arg(m_cursor->pos().y()), 10000); } // timeout is 10s
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

void GraphicsView::keyPressEvent(QKeyEvent* event)
{
    while (true)
    {
        if (event->key() == Qt::Key_1)
        {
            if (!m_cursor) break;
            if (!m_position1)
            {
                MarkerItem::MarkerShape shape = MarkerItem::Circle;
                qreal size = 10.0;
                QColor colour = Qt::green;
                m_position1 = new MarkerItem(m_cursor->pos(), shape, size, colour);
                scene()->addItem(m_position1);
            }
            else
            {
                m_position1->setPos(m_cursor->pos());
            }
            QMainWindow* mainWin = qobject_cast<QMainWindow*>(this->window());
            if (mainWin) { mainWin->statusBar()->showMessage(QString("Position 1 x = %1 y = %2").arg(m_position1->pos().x()).arg(m_position1->pos().y()), 10000); }
            break;
        }
        if (event->key() == Qt::Key_2)
        {
            if (!m_cursor) break;
            if (!m_position2)
            {
                MarkerItem::MarkerShape shape = MarkerItem::Circle;
                qreal size = 10.0;
                QColor colour = Qt::blue;
                m_position2 = new MarkerItem(m_cursor->pos(), shape, size, colour);
                scene()->addItem(m_position2);
            }
            else
            {
                m_position2->setPos(m_cursor->pos());
            }
            QMainWindow* mainWin = qobject_cast<QMainWindow*>(this->window());
            if (mainWin) { mainWin->statusBar()->showMessage(QString("Position 2 x = %1 y = %2").arg(m_position2->pos().x()).arg(m_position2->pos().y()), 10000); }
            break;
        }
        QGraphicsView::keyPressEvent(event); // Pass to base class
        break;
    }
}

