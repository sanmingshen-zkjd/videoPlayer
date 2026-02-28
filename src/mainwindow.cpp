#include "mainwindow.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEvent>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QMediaContent>
#include <QMediaPlayer>
#include <QMenuBar>
#include <QMimeDatabase>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

namespace {

class ImageAdjustDialog : public QDialog {
    Q_OBJECT
public:
    explicit ImageAdjustDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle(tr("Brightness / Contrast"));
        setModal(false);
        auto *layout = new QFormLayout(this);
        m_brightness = new QSlider(Qt::Horizontal, this);
        m_brightness->setRange(-100, 100);
        layout->addRow(tr("Brightness"), m_brightness);
        m_contrast = new QSlider(Qt::Horizontal, this);
        m_contrast->setRange(-100, 100);
        layout->addRow(tr("Contrast"), m_contrast);
        connect(m_brightness, &QSlider::valueChanged, this, &ImageAdjustDialog::adjustChanged);
        connect(m_contrast, &QSlider::valueChanged, this, &ImageAdjustDialog::adjustChanged);
    }
    int brightness() const { return m_brightness->value(); }
    int contrast() const { return m_contrast->value(); }
signals:
    void adjustChanged();
private:
    QSlider *m_brightness;
    QSlider *m_contrast;
};

class PointConfigDialog : public QDialog {
    Q_OBJECT
public:
    PointConfigDialog(const QPointF &scenePos, const QImage &aroundImage, QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle(tr("Point Configuration"));
        auto *layout = new QVBoxLayout(this);
        layout->addWidget(new QLabel(tr("Point: (%1, %2)").arg(scenePos.x(), 0, 'f', 1).arg(scenePos.y(), 0, 'f', 1), this));
        auto *content = new QHBoxLayout();
        auto *previewLabel = new QLabel(this);
        previewLabel->setFixedSize(220, 220);
        previewLabel->setFrameShape(QFrame::StyledPanel);
        previewLabel->setAlignment(Qt::AlignCenter);
        if (!aroundImage.isNull()) previewLabel->setPixmap(QPixmap::fromImage(aroundImage).scaled(previewLabel->size(), Qt::KeepAspectRatio));
        content->addWidget(previewLabel);
        auto *form = new QFormLayout();
        m_name = new QLineEdit(tr("Point"), this);
        m_algorithm = new QComboBox(this);
        m_algorithm->addItems({tr("Image Correlation"), tr("Centroid"), tr("Template Tracking"), tr("Kalman")});
        m_searchRadius = new QSpinBox(this);
        m_searchRadius->setRange(10, 300);
        m_searchRadius->setValue(100);
        m_threshold = new QSpinBox(this);
        m_threshold->setRange(1, 255);
        m_threshold->setValue(25);
        form->addRow(tr("Name"), m_name);
        form->addRow(tr("Algorithm"), m_algorithm);
        form->addRow(tr("Search Radius"), m_searchRadius);
        form->addRow(tr("Threshold"), m_threshold);
        content->addLayout(form, 1);
        layout->addLayout(content);
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        layout->addWidget(buttons);
    }
    QString name() const { return m_name->text().trimmed().isEmpty() ? QStringLiteral("Point") : m_name->text().trimmed(); }
    QString algorithm() const { return m_algorithm->currentText(); }
    int searchRadius() const { return m_searchRadius->value(); }
    int threshold() const { return m_threshold->value(); }
private:
    QLineEdit *m_name;
    QComboBox *m_algorithm;
    QSpinBox *m_searchRadius;
    QSpinBox *m_threshold;
};

}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_canvas(new PlaybackCanvas(this)),
      m_player(new QMediaPlayer(this)),
      m_imageTimer(new QTimer(this)),
      m_timeline(new QSlider(Qt::Horizontal, this)),
      m_rateCombo(new QComboBox(this)),
      m_quickToolBar(nullptr),
      m_sequenceIndex(0),
      m_currentMediaKind(MediaKind::None),
      m_adjustDialog(nullptr),
      m_brightness(0),
      m_contrast(0),
      m_playbackRate(1.0) {
    setupUi();
    setupConnections();
    m_canvas->attachPlayer(m_player);
    m_imageTimer->setInterval(1000 / 24);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_canvas && event->type() == QEvent::Resize && m_quickToolBar) {
        m_quickToolBar->move(12, m_canvas->height() - m_quickToolBar->height() - 12);
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::setupUi() {
    resize(1280, 820);
    buildMenuBar();
    buildMainToolBar();

    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    layout->addWidget(m_canvas, 1);

    auto *playbackPanel = new QWidget(this);
    auto *playbackLayout = new QHBoxLayout(playbackPanel);
    auto *rewindButton = new QPushButton(tr("<< 5s"), playbackPanel);
    auto *playButton = new QPushButton(tr("Play"), playbackPanel);
    auto *pauseButton = new QPushButton(tr("Pause"), playbackPanel);
    auto *stopButton = new QPushButton(tr("Stop"), playbackPanel);
    auto *forwardButton = new QPushButton(tr("5s >>"), playbackPanel);
    m_rateCombo->addItems({"0.25x", "0.5x", "1.0x", "2.0x", "4.0x", "6.0x"});
    m_rateCombo->setCurrentText("1.0x");
    connect(rewindButton, &QPushButton::clicked, this, &MainWindow::rewind);
    connect(playButton, &QPushButton::clicked, this, &MainWindow::play);
    connect(pauseButton, &QPushButton::clicked, this, &MainWindow::pause);
    connect(stopButton, &QPushButton::clicked, this, &MainWindow::stop);
    connect(forwardButton, &QPushButton::clicked, this, &MainWindow::fastForward);
    playbackLayout->addWidget(rewindButton);
    playbackLayout->addWidget(playButton);
    playbackLayout->addWidget(pauseButton);
    playbackLayout->addWidget(stopButton);
    playbackLayout->addWidget(forwardButton);
    playbackLayout->addWidget(new QLabel(tr("Rate"), playbackPanel));
    playbackLayout->addWidget(m_rateCombo);
    playbackLayout->addWidget(m_timeline, 1);
    layout->addWidget(playbackPanel);

    setCentralWidget(central);
    buildQuickToolBar();
}

void MainWindow::buildMenuBar() {
    auto *fileMenu = menuBar()->addMenu(tr("File"));
    fileMenu->addAction(tr("Import"), this, &MainWindow::importMedia);
    auto *viewMenu = menuBar()->addMenu(tr("View"));
    viewMenu->addAction(tr("Zoom In"), m_canvas, &PlaybackCanvas::zoomIn);
    viewMenu->addAction(tr("Zoom Out"), m_canvas, &PlaybackCanvas::zoomOut);
    viewMenu->addAction(tr("Reset View"), m_canvas, &PlaybackCanvas::resetViewTransform);
    viewMenu->addAction(tr("Brightness/Contrast"), this, &MainWindow::openImageAdjustDialog);
    auto *drawMenu = menuBar()->addMenu(tr("Draw"));
    drawMenu->addAction(tr("Draw Point"), [this]() { m_canvas->setDrawMode(PlaybackCanvas::DrawMode::Point); });
    drawMenu->addAction(tr("Draw Line"), [this]() { m_canvas->setDrawMode(PlaybackCanvas::DrawMode::Line); });
    drawMenu->addAction(tr("Clear Drawings"), m_canvas, &PlaybackCanvas::clearDrawings);
}

void MainWindow::buildMainToolBar() {
    auto *tb = addToolBar(tr("Main"));
    tb->addAction(tr("Import"), this, &MainWindow::importMedia);
    tb->addAction(tr("Zoom In"), m_canvas, &PlaybackCanvas::zoomIn);
    tb->addAction(tr("Zoom Out"), m_canvas, &PlaybackCanvas::zoomOut);
    tb->addAction(tr("Draw Point"), [this]() { m_canvas->setDrawMode(PlaybackCanvas::DrawMode::Point); });
    tb->addAction(tr("Draw Line"), [this]() { m_canvas->setDrawMode(PlaybackCanvas::DrawMode::Line); });
    tb->addAction(tr("Clear"), m_canvas, &PlaybackCanvas::clearDrawings);
}

void MainWindow::buildQuickToolBar() {
    m_quickToolBar = new QToolBar(tr("Quick"), m_canvas);
    m_quickToolBar->setMovable(false);
    m_quickToolBar->setFloatable(false);
    m_quickToolBar->setStyleSheet("QToolBar { background: rgba(25, 25, 25, 150); color: white; border: 1px solid rgba(255,255,255,80); }");
    m_quickToolBar->addAction(tr("Point"), [this]() { m_canvas->setDrawMode(PlaybackCanvas::DrawMode::Point); });
    m_quickToolBar->addAction(tr("Line"), [this]() { m_canvas->setDrawMode(PlaybackCanvas::DrawMode::Line); });
    m_quickToolBar->addAction(tr("Clear"), m_canvas, &PlaybackCanvas::clearDrawings);
    m_quickToolBar->adjustSize();
    m_quickToolBar->move(12, m_canvas->height() - m_quickToolBar->height() - 12);
    m_quickToolBar->show();
    m_canvas->installEventFilter(this);
}

void MainWindow::setupConnections() {
    connect(m_player, &QMediaPlayer::positionChanged, this, &MainWindow::onPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &MainWindow::onDurationChanged);
    connect(m_timeline, &QSlider::sliderMoved, this, &MainWindow::seek);
    connect(m_imageTimer, &QTimer::timeout, this, &MainWindow::nextImageFrame);
    connect(m_rateCombo, &QComboBox::currentTextChanged, this, &MainWindow::onPlaybackRateChanged);
    connect(m_canvas, &PlaybackCanvas::pointDrawn, this, &MainWindow::onPointDrawn);
}

void MainWindow::importMedia() {
    const QStringList files = QFileDialog::getOpenFileNames(this, tr("Import"), QString(), tr("Media Files (*.mp4 *.mov *.avi *.mkv *.wmv *.flv *.webm *.png *.jpg *.jpeg *.bmp *.tif *.tiff)"));
    if (files.isEmpty()) return;
    bool allImages = true;
    for (const QString &filePath : files) {
        if (!isImageFile(filePath)) { allImages = false; break; }
    }
    if (allImages) loadImageSequence(files);
    else loadVideo(files.first());
}

void MainWindow::play() { if (m_currentMediaKind == MediaKind::ImageSequence) m_imageTimer->start(); else m_player->play(); }
void MainWindow::pause() { if (m_currentMediaKind == MediaKind::ImageSequence) m_imageTimer->stop(); else m_player->pause(); }
void MainWindow::stop() { if (m_currentMediaKind == MediaKind::ImageSequence) { m_imageTimer->stop(); m_sequenceIndex = 0; showImageAt(m_sequenceIndex);} else m_player->stop(); }
void MainWindow::fastForward() { if (m_currentMediaKind == MediaKind::ImageSequence) { m_sequenceIndex = qMin(m_sequenceIndex + 10, m_sequenceFiles.size() - 1); showImageAt(m_sequenceIndex);} else m_player->setPosition(m_player->position() + 5000); }
void MainWindow::rewind() { if (m_currentMediaKind == MediaKind::ImageSequence) { m_sequenceIndex = qMax(m_sequenceIndex - 10, 0); showImageAt(m_sequenceIndex);} else m_player->setPosition(qMax<qint64>(m_player->position() - 5000, 0)); }
void MainWindow::onPositionChanged(qint64 position) { if (m_currentMediaKind == MediaKind::Video) m_timeline->setValue(static_cast<int>(position)); }
void MainWindow::onDurationChanged(qint64 duration) { if (m_currentMediaKind == MediaKind::Video) m_timeline->setRange(0, static_cast<int>(duration)); }
void MainWindow::seek(int value) { if (m_currentMediaKind == MediaKind::ImageSequence) { m_sequenceIndex = value; showImageAt(m_sequenceIndex);} else m_player->setPosition(value); }
void MainWindow::nextImageFrame() { if (!m_sequenceFiles.isEmpty()) { m_sequenceIndex = (m_sequenceIndex + 1) % m_sequenceFiles.size(); showImageAt(m_sequenceIndex);} }

void MainWindow::openImageAdjustDialog() {
    if (!m_adjustDialog) {
        auto *dialog = new ImageAdjustDialog(this);
        m_adjustDialog = dialog;
        connect(dialog, &ImageAdjustDialog::adjustChanged, this, [this, dialog]() {
            m_brightness = dialog->brightness();
            m_contrast = dialog->contrast();
            applyImageAdjustments();
        });
    }
    m_adjustDialog->show();
}

void MainWindow::onPlaybackRateChanged(const QString &text) {
    QString normalized = text;
    normalized.remove('x');
    bool ok = false;
    const double rate = normalized.toDouble(&ok);
    if (ok) setPlaybackRate(rate);
}

void MainWindow::onPointDrawn(const QPointF &scenePos, const QImage &aroundImage) {
    PointConfigDialog dialog(scenePos, aroundImage, this);
    if (dialog.exec() == QDialog::Accepted) {
        const QImage reference = m_canvas->grab().toImage();
        m_canvas->addTrackedPoint(scenePos, dialog.name(), dialog.algorithm(), dialog.searchRadius(), dialog.threshold(), reference);
    }
}

void MainWindow::loadVideo(const QString &filePath) {
    m_currentMediaKind = MediaKind::Video;
    m_imageTimer->stop();
    m_sequenceFiles.clear();
    m_canvas->clearImage();
    m_player->setMedia(QUrl::fromLocalFile(filePath));
    m_player->pause();
    m_player->setPosition(0);
    m_timeline->setRange(0, 0);
    applyImageAdjustments();
}

void MainWindow::loadImageSequence(const QStringList &files) {
    m_currentMediaKind = MediaKind::ImageSequence;
    m_player->stop();
    m_player->setMedia(QMediaContent());
    m_sequenceFiles = files;
    m_sequenceIndex = 0;
    m_timeline->setRange(0, m_sequenceFiles.size() - 1);
    showImageAt(m_sequenceIndex);
    applyImageAdjustments();
}

void MainWindow::showImageAt(int index) {
    if (index < 0 || index >= m_sequenceFiles.size()) return;
    QImage image(m_sequenceFiles.at(index));
    if (!image.isNull()) {
        m_canvas->setImage(image);
        m_canvas->updateTracking(image);
        m_timeline->setValue(index);
    }
}

bool MainWindow::isImageFile(const QString &filePath) const {
    QMimeDatabase db;
    return db.mimeTypeForFile(filePath, QMimeDatabase::MatchContent).name().startsWith("image/");
}

void MainWindow::applyImageAdjustments() { m_canvas->setBrightnessContrast(m_brightness, m_contrast); }

void MainWindow::setPlaybackRate(double rate) {
    m_playbackRate = rate;
    if (m_currentMediaKind == MediaKind::ImageSequence) {
        m_imageTimer->setInterval(qMax(1, static_cast<int>(1000.0 / (24.0 * m_playbackRate))));
    }
    m_player->setPlaybackRate(m_playbackRate);
}

#include "mainwindow.moc"
