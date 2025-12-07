#ifndef GRAPHICSVIEW_H
#define GRAPHICSVIEW_H

#include <QGraphicsView>

class QWheelEvent;
class QMouseEvent;
class QKeyEvent;
class MarkerItem;

class GraphicsView : public QGraphicsView
{
public:
    GraphicsView(QGraphicsScene *scene, QWidget *parent = nullptr);

    void clear();

    MarkerItem *cursor() const;

    MarkerItem *position1() const;

    MarkerItem *position2() const;

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
};

#endif // GRAPHICSVIEW_H
