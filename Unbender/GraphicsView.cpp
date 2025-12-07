#include "GraphicsView.h"
#include "MarkerItem.h"
#include "MainWindow.h"

#include <QWheelEvent>
#include <QGraphicsItem>
#include <QStatusBar>
#include <QMainWindow>

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
            if (QMainWindow* mainWindow = qobject_cast<QMainWindow*>(this->window())) { mainWindow->statusBar()->showMessage(QString("Cursor x = %1 y = %2").arg(m_cursor->pos().x()).arg(m_cursor->pos().y()), 10000); }
            if (MainWindow* mainWindow = qobject_cast<MainWindow*>(this->window())) { mainWindow->updateUI(); }
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
            if (QMainWindow* mainWindow = qobject_cast<QMainWindow*>(this->window())) { mainWindow->statusBar()->showMessage(QString("Position 1 x = %1 y = %2").arg(m_position1->pos().x()).arg(m_position1->pos().y()), 10000); }
            if (MainWindow* mainWindow = qobject_cast<MainWindow*>(this->window())) { mainWindow->updateUI(); }
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
            if (QMainWindow* mainWindow = qobject_cast<QMainWindow*>(this->window())) { mainWindow->statusBar()->showMessage(QString("Position 2 x = %1 y = %2").arg(m_position2->pos().x()).arg(m_position2->pos().y()), 10000); }
            if (MainWindow* mainWindow = qobject_cast<MainWindow*>(this->window())) { mainWindow->updateUI(); }
            break;
        }
        QGraphicsView::keyPressEvent(event); // Pass to base class
        break;
    }
}

void GraphicsView::clear()
{
    m_cursor = 0;
    m_position1 = 0;
    m_position2 = 0;
}

MarkerItem *GraphicsView::position2() const
{
    return m_position2;
}

MarkerItem *GraphicsView::position1() const
{
    return m_position1;
}

MarkerItem *GraphicsView::cursor() const
{
    return m_cursor;
}

