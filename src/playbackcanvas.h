#pragma once

#include <QColor>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsVideoItem>
#include <QGraphicsView>
#include <QLineF>
#include <QPointF>
#include <QVector>

class QMediaPlayer;
class QMouseEvent;
class QPainter;
class QWheelEvent;

class PlaybackCanvas : public QGraphicsView {
    Q_OBJECT

public:
    enum class DrawMode {
        None,
        Point,
        Line
    };

    explicit PlaybackCanvas(QWidget *parent = nullptr);

    void attachPlayer(QMediaPlayer *player);
    void setImage(const QImage &image);
    void clearImage();

    void zoomIn();
    void zoomOut();
    void resetViewTransform();

    void setDrawMode(DrawMode mode);
    void clearDrawings();
    void setBrightnessContrast(int brightness, int contrast);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void drawForeground(QPainter *painter, const QRectF &rect) override;

private:
    class BrightnessContrastEffect;

    QGraphicsScene m_scene;
    QGraphicsPixmapItem *m_imageItem;
    QGraphicsVideoItem *m_videoItem;
    BrightnessContrastEffect *m_imageEffect;
    BrightnessContrastEffect *m_videoEffect;

    DrawMode m_drawMode;
    QVector<QPointF> m_points;
    QVector<QLineF> m_lines;

    bool m_waitingForSecondPoint;
    QPointF m_firstPoint;
    QPointF m_hoverPoint;

    QColor m_lineColor;
    qreal m_lineWidth;

    void applyZoomFactor(double factor);
    void openLineStyleDialog();
};
