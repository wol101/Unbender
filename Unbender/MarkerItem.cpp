#include "MarkerItem.h"

MarkerItem::MarkerItem(QPointF center, MarkerShape shape, qreal size, QColor color)
    : m_shape(shape), m_size(size), m_color(color)
{
    setPos(center); // position in scene coordinates
}

QRectF MarkerItem::boundingRect() const
{
    return QRectF(-m_size/2, -m_size/2, m_size, m_size);
}

void MarkerItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QPen(m_color, 2));
    painter->setBrush(Qt::NoBrush);

    switch (m_shape)
    {
    case Circle:
        painter->drawEllipse(QPointF(0,0), m_size/2, m_size/2);
        break;
    case Cross:
        painter->drawLine(QPointF(-m_size/2, 0), QPointF(m_size/2, 0));
        painter->drawLine(QPointF(0, -m_size/2), QPointF(0, m_size/2));
        break;
    case Square:
        painter->drawRect(-m_size/2, -m_size/2, m_size, m_size);
        break;
    case Triangle:
        {
            QPolygonF tri;
            tri << QPointF(0, -m_size/2) << QPointF(-m_size/2, m_size/2) << QPointF(m_size/2, m_size/2);
            painter->drawPolygon(tri);
            break;
        }
    }
}

void MarkerItem::setShape(MarkerShape shape) { m_shape = shape; update(); }
void MarkerItem::setSize(qreal size) { m_size = size; prepareGeometryChange(); }
void MarkerItem::setColor(QColor color) { m_color = color; update(); }


