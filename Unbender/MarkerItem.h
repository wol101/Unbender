#ifndef MARKERITEM_H
#define MARKERITEM_H

#include <QGraphicsItem>
#include <QPainter>

class MarkerItem : public QGraphicsItem
{
public:
    enum MarkerShape {
        Circle,
        Cross,
        Square,
        Triangle
    };
    MarkerItem(QPointF center, MarkerShape shape = Circle, qreal size = 10.0, QColor colour = Qt::red);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;

    void setShape(MarkerShape shape);
    void setSize(qreal size);
    void setColor(QColor colour);

private:
    MarkerShape m_shape;
    qreal m_size;
    QColor m_colour;

};

#endif // MARKERITEM_H

