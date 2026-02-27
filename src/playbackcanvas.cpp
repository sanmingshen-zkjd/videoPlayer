#include "playbackcanvas.h"

#include <QColorDialog>
#include <QCursor>
#include <QGraphicsEffect>
#include <QInputDialog>
#include <QMediaPlayer>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QWheelEvent>

class PlaybackCanvas::BrightnessContrastEffect : public QGraphicsEffect {
public:
    BrightnessContrastEffect(QObject *parent = nullptr)
        : QGraphicsEffect(parent), m_brightness(0), m_contrast(0) {}

    void setValues(int brightness, int contrast) {
        m_brightness = qBound(-100, brightness, 100);
        m_contrast = qBound(-100, contrast, 100);
        update();
    }

protected:
    void draw(QPainter *painter) override {
        QPoint offset;
        const QPixmap src = sourcePixmap(Qt::LogicalCoordinates, &offset, NoPad);
        if (src.isNull()) {
            return;
        }

        QImage image = src.toImage().convertToFormat(QImage::Format_ARGB32);
        const double contrastFactor = (m_contrast + 100) / 100.0;

        for (int y = 0; y < image.height(); ++y) {
            QRgb *line = reinterpret_cast<QRgb *>(image.scanLine(y));
            for (int x = 0; x < image.width(); ++x) {
                const QColor srcColor(line[x]);
                const int r = qBound(0, static_cast<int>(((srcColor.red() - 127) * contrastFactor) + 127 + m_brightness), 255);
                const int g = qBound(0, static_cast<int>(((srcColor.green() - 127) * contrastFactor) + 127 + m_brightness), 255);
                const int b = qBound(0, static_cast<int>(((srcColor.blue() - 127) * contrastFactor) + 127 + m_brightness), 255);
                line[x] = qRgba(r, g, b, srcColor.alpha());
            }
        }

        painter->drawImage(offset, image);
    }

private:
    int m_brightness;
    int m_contrast;
};

namespace {
QCursor highContrastCrossCursor() {
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPen outline(Qt::black);
    outline.setWidth(3);
    painter.setPen(outline);
    painter.drawLine(16, 2, 16, 30);
    painter.drawLine(2, 16, 30, 16);

    QPen inner(Qt::yellow);
    inner.setWidth(1);
    painter.setPen(inner);
    painter.drawLine(16, 2, 16, 30);
    painter.drawLine(2, 16, 30, 16);

    painter.end();
    return QCursor(pixmap, 16, 16);
}
} // namespace

PlaybackCanvas::PlaybackCanvas(QWidget *parent)
    : QGraphicsView(parent),
      m_scene(this),
      m_imageItem(new QGraphicsPixmapItem()),
      m_videoItem(new QGraphicsVideoItem()),
      m_imageEffect(new BrightnessContrastEffect(this)),
      m_videoEffect(new BrightnessContrastEffect(this)),
      m_drawMode(DrawMode::None),
      m_waitingForSecondPoint(false),
      m_lineColor(QColor(64, 200, 255)),
      m_lineWidth(2.0) {
    setScene(&m_scene);

    m_scene.addItem(m_videoItem);
    m_scene.addItem(m_imageItem);

    m_videoItem->setAspectRatioMode(Qt::KeepAspectRatio);
    m_videoItem->setSize(QSizeF(1280, 720));
    m_videoItem->setGraphicsEffect(m_videoEffect);

    m_imageItem->setVisible(false);
    m_imageItem->setGraphicsEffect(m_imageEffect);

    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::SmoothPixmapTransform, true);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setCursor(Qt::OpenHandCursor);
    setMouseTracking(true);

    m_scene.setSceneRect(QRectF(0, 0, 1280, 720));
}

void PlaybackCanvas::attachPlayer(QMediaPlayer *player) {
    if (player) {
        player->setVideoOutput(m_videoItem);
    }
}

void PlaybackCanvas::setImage(const QImage &image) {
    m_imageItem->setPixmap(QPixmap::fromImage(image));
    m_imageItem->setVisible(true);
    m_videoItem->setVisible(false);

    const QRectF bounds = m_imageItem->boundingRect();
    if (!bounds.isEmpty()) {
        m_scene.setSceneRect(bounds);
        fitInView(bounds, Qt::KeepAspectRatio);
    }
}

void PlaybackCanvas::clearImage() {
    m_imageItem->setVisible(false);
    m_videoItem->setVisible(true);
    m_scene.setSceneRect(QRectF(0, 0, 1280, 720));
}

void PlaybackCanvas::zoomIn() {
    applyZoomFactor(1.2);
}

void PlaybackCanvas::zoomOut() {
    applyZoomFactor(1.0 / 1.2);
}

void PlaybackCanvas::resetViewTransform() {
    resetTransform();
    fitInView(m_scene.sceneRect(), Qt::KeepAspectRatio);
}

void PlaybackCanvas::setDrawMode(DrawMode mode) {
    m_drawMode = mode;
    m_waitingForSecondPoint = false;

    if (m_drawMode == DrawMode::None) {
        setDragMode(QGraphicsView::ScrollHandDrag);
        setCursor(Qt::OpenHandCursor);
    } else {
        setDragMode(QGraphicsView::NoDrag);
        setCursor(highContrastCrossCursor());
    }
    viewport()->update();
}

void PlaybackCanvas::clearDrawings() {
    m_points.clear();
    m_lines.clear();
    m_waitingForSecondPoint = false;
    viewport()->update();
}

void PlaybackCanvas::setBrightnessContrast(int brightness, int contrast) {
    m_imageEffect->setValues(brightness, contrast);
    m_videoEffect->setValues(brightness, contrast);
    viewport()->update();
}

void PlaybackCanvas::wheelEvent(QWheelEvent *event) {
    const double factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
    applyZoomFactor(factor);
    event->accept();
}

void PlaybackCanvas::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::RightButton && m_drawMode == DrawMode::Line) {
        openLineStyleDialog();
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton && m_drawMode != DrawMode::None) {
        const QPointF scenePos = mapToScene(event->pos());
        if (m_drawMode == DrawMode::Point) {
            m_points.push_back(scenePos);
            setDrawMode(DrawMode::None);
        } else if (m_drawMode == DrawMode::Line) {
            if (!m_waitingForSecondPoint) {
                m_firstPoint = scenePos;
                m_hoverPoint = scenePos;
                m_waitingForSecondPoint = true;
            } else {
                m_lines.push_back(QLineF(m_firstPoint, scenePos));
                m_waitingForSecondPoint = false;
                setDrawMode(DrawMode::None);
            }
        }
        viewport()->update();
        event->accept();
        return;
    }

    QGraphicsView::mousePressEvent(event);
}

void PlaybackCanvas::mouseMoveEvent(QMouseEvent *event) {
    if (m_drawMode == DrawMode::Line && m_waitingForSecondPoint) {
        m_hoverPoint = mapToScene(event->pos());
        viewport()->update();
    }
    QGraphicsView::mouseMoveEvent(event);
}

void PlaybackCanvas::drawForeground(QPainter *painter, const QRectF &rect) {
    Q_UNUSED(rect)

    QPen pointPen(QColor(255, 64, 64));
    pointPen.setWidthF(4.0);
    painter->setPen(pointPen);
    for (const QPointF &point : m_points) {
        painter->drawPoint(point);
    }

    QPen linePen(m_lineColor);
    linePen.setWidthF(m_lineWidth);
    painter->setPen(linePen);
    for (const QLineF &line : m_lines) {
        painter->drawLine(line);
    }

    if (m_drawMode == DrawMode::Line && m_waitingForSecondPoint) {
        QPen previewPen(m_lineColor);
        previewPen.setWidthF(m_lineWidth);
        previewPen.setStyle(Qt::DashLine);
        painter->setPen(previewPen);
        painter->drawLine(QLineF(m_firstPoint, m_hoverPoint));
    }
}

void PlaybackCanvas::applyZoomFactor(double factor) {
    scale(factor, factor);
}

void PlaybackCanvas::openLineStyleDialog() {
    bool ok = false;
    const double width = QInputDialog::getDouble(
        this,
        tr("Line Width"),
        tr("Width:"),
        m_lineWidth,
        1.0,
        20.0,
        1,
        &ok
    );
    if (ok) {
        m_lineWidth = width;
    }

    const QColor picked = QColorDialog::getColor(m_lineColor, this, tr("Line Color"));
    if (picked.isValid()) {
        m_lineColor = picked;
    }

    viewport()->update();
}
