#pragma once

#include <QColor>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsVideoItem>
#include <QGraphicsView>
#include <QMap>

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
    void addTrackedPoint(const QPointF &scenePos,
                         const QString &name,
                         const QString &algorithm,
                         int searchRadius,
                         int threshold,
                         const QImage &referenceFrame);
    void updateTracking(const QImage &frame);

signals:
    void pointDrawn(const QPointF &scenePos, const QImage &aroundImage);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void drawForeground(QPainter *painter, const QRectF &rect) override;

private:
    class BrightnessContrastEffect;
    class NamedPointItem;
    class NamedLineItem;

    struct TrackInfo {
        NamedPointItem *item;
        QString algorithm;
        int searchRadius;
        int threshold;
        QImage templatePatch;
        QPointF lastPos;
    };

    QGraphicsScene m_scene;
    QGraphicsPixmapItem *m_imageItem;
    QGraphicsVideoItem *m_videoItem;
    BrightnessContrastEffect *m_imageEffect;
    BrightnessContrastEffect *m_videoEffect;

    DrawMode m_drawMode;

    bool m_waitingForSecondPoint;
    QPointF m_firstPoint;
    QPointF m_hoverPoint;

    QColor m_lineColor;
    qreal m_lineWidth;

    int m_pointCounter;
    int m_lineCounter;
    QMap<int, TrackInfo> m_tracks;
    int m_nextTrackId;

    void applyZoomFactor(double factor);
    void openLineStyleDialog();
    QImage captureAroundViewPos(const QPoint &viewPos, int halfSize = 100) const;
    QImage extractPatch(const QImage &img, const QPointF &scenePos, int half = 6) const;
};
