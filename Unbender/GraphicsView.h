#ifndef GRAPHICSVIEW_H
#define GRAPHICSVIEW_H

#include <QGraphicsView>

class QWheelEvent;
class QMouseEvent;
class QKeyEvent;
class MarkerItem;
class QGraphicsPixmapItem;
class QGraphicsPathItem;

class GraphicsView : public QGraphicsView
{
public:
    GraphicsView(QGraphicsScene *scene, QWidget *parent = nullptr);

    void clear();

    void addExtraItem(QGraphicsItem *item);

    void clearExtrasItems();

    void setImage(QGraphicsPixmapItem *pixmapItem);

    void setOutline(QGraphicsPathItem *outline);

    QImage renderSceneToImage();

    MarkerItem *cursor() const;

    MarkerItem *position1() const;

    MarkerItem *position2() const;

    void setPosition1(QPointF center);

    void setPosition2(QPointF center);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    MarkerItem *m_cursor = 0;
    MarkerItem *m_position1 = 0;
    MarkerItem *m_position2 = 0;
    QGraphicsPixmapItem *m_image = 0;
    QGraphicsPathItem *m_outline = 0;
    std::vector<QGraphicsItem *> m_extraItems;

    bool panning = false;
    QPoint panStart;

};

#endif // GRAPHICSVIEW_H
