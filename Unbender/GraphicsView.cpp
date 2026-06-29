#include "GraphicsView.h"
#include "MarkerItem.h"
#include "MainWindow.h"

#include <QWheelEvent>
#include <QGraphicsItem>
#include <QStatusBar>
#include <QMainWindow>
#include <QScrollBar>

GraphicsView::GraphicsView(QGraphicsScene *scene, QWidget *parent) : QGraphicsView(scene, parent)
{
    // setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    // Optional: smoother visual quality
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);
    setMouseTracking(true); // needed to get mouse move events all the time
}

void GraphicsView::wheelEvent(QWheelEvent *event)
{
    constexpr double zoomInFactor  = 1.15;
    constexpr double zoomOutFactor = 1.0 / zoomInFactor;

    double factor = (event->angleDelta().y() > 0) ? zoomInFactor : zoomOutFactor;

    // Optional: clamp total zoom
    const double currentScale = transform().m11(); // assumes uniform scaling
    const double minScale = 0.05;
    const double maxScale = 50.0;

    double newScale = currentScale * factor;
    if (newScale < minScale)
        factor = minScale / currentScale;
    else if (newScale > maxScale)
        factor = maxScale / currentScale;

    scale(factor, factor);

}

void GraphicsView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_image)
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
        event->accept();
        return;
    }
    if (event->button() == Qt::MiddleButton  && m_image)
    {
        panning = true;
        panStart = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    QGraphicsView::mousePressEvent(event);
}

void GraphicsView::mouseMoveEvent(QMouseEvent *event)
{
    if (panning && m_image)
    {
        QPoint delta = event->pos() - panStart;
        panStart = event->pos();

        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());

        event->accept();
        return;
    }

    if (m_image)
    {
        // Map mouse position from view → scene → item
        QPointF scenePos = mapToScene(event->pos());
        QPointF itemPos  = m_image->mapFromScene(scenePos);

        int x = static_cast<int>(itemPos.x());
        int y = static_cast<int>(itemPos.y());

        // Bounds check
        if (x >= 0 && y >= 0 && x < m_image->pixmap().width() && y < m_image->pixmap().height())
        {
            // Convert pixmap to QImage and query pixel
            QImage img = m_image->pixmap().toImage();
            QColor color = img.pixelColor(x, y);
            if (QMainWindow* mainWindow = qobject_cast<QMainWindow*>(this->window())) { mainWindow->statusBar()->showMessage(QString("x=%1 y=%2 R=%3 G=%4 B=%5 A=%6").arg(x).arg(y).arg(color.red()).arg(color.green()).arg(color.blue()).arg(color.alpha())); }
        }
    }

    QGraphicsView::mouseMoveEvent(event);

}

void GraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton)
    {
        panning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    QGraphicsView::mouseReleaseEvent(event);

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
            emit uiUpdateRequested();
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
            emit uiUpdateRequested();
            break;
        }
        QGraphicsView::keyPressEvent(event); // Pass to base class
        break;
    }
}

void GraphicsView::clear()
{
    scene()->clear();
    m_extraItems.clear();
    m_cursor = 0;
    m_position1 = 0;
    m_position2 = 0;
    m_image = 0;
    m_outline = 0;
}

void GraphicsView::addExtraItem(QGraphicsItem *item)
{
    if (item)
    {
        m_extraItems.push_back(item);
        scene()->addItem(item);
    }
}

void GraphicsView::clearExtrasItems()
{
    for (auto &&item : m_extraItems)
    {
        scene()->removeItem(item);
        delete item;
    }
    m_extraItems.clear();
}

void GraphicsView::setImage(QGraphicsPixmapItem *pixmapItem)
{
    if (m_image)
    {
        scene()->removeItem(m_image);
        delete m_image;
    }
    m_image = pixmapItem;
    if (m_image) { scene()->addItem(m_image); }
}

void GraphicsView::setOutline(QGraphicsPathItem *pathItem)
{
    if (m_outline)
    {
        scene()->removeItem(m_outline);
        delete m_outline;
    }
    m_outline = pathItem;
    if (m_outline)
    {
        m_outline->setPen(QPen(Qt::magenta, 2));
        m_outline->setBrush(Qt::NoBrush);
        scene()->addItem(m_outline);
    }
}

QImage GraphicsView::renderSceneToImage()
{
    // Get the pixmap's rect in scene coordinates
    QRectF clipRect = m_image->mapToScene(m_image->boundingRect()).boundingRect();

    // Create a QImage at the pixmap's native resolution
    QSize imageSize = m_image->pixmap().size();
    QImage image(imageSize, QImage::Format_RGBA8888);
    image.fill(Qt::transparent);

    // Paint the scene onto the image, mapped to the pixmap rect
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    scene()->render(&painter,
                  QRectF(QPointF(0, 0), imageSize),  // target: full image
                  clipRect);                         // source: pixmap bounds in scene

    return image;
}

void GraphicsView::setPosition1(QPointF center)
{
    if (!m_position1)
    {
        MarkerItem::MarkerShape shape = MarkerItem::Circle;
        qreal size = 10.0;
        QColor colour = Qt::green;
        m_position1 = new MarkerItem(center, shape, size, colour);
        scene()->addItem(m_position1);
    }
    else
    {
        m_position1->setPos(center);
    }
}

void GraphicsView::setPosition2(QPointF center)
{
    if (!m_position2)
    {
        MarkerItem::MarkerShape shape = MarkerItem::Circle;
        qreal size = 10.0;
        QColor colour = Qt::green;
        m_position1 = new MarkerItem(center, shape, size, colour);
        scene()->addItem(m_position2);
    }
    else
    {
        m_position2->setPos(center);
    }
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

