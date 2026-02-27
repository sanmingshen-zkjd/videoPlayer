#include "mainwindow.h"

#include <QFileDialog>
#include <QMediaContent>
#include <QMediaPlayer>
#include <QMimeDatabase>
#include <QSlider>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_canvas(new PlaybackCanvas(this)),
      m_player(new QMediaPlayer(this)),
      m_imageTimer(new QTimer(this)),
      m_timeline(new QSlider(Qt::Horizontal, this)),
      m_sequenceIndex(0),
      m_currentMediaKind(MediaKind::None) {
    setupUi();
    setupConnections();

    m_canvas->attachPlayer(m_player);

    m_imageTimer->setInterval(1000 / 24);

    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::setupUi() {
    setWindowTitle(tr("Qt C++ Player"));
    resize(1200, 800);

    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    m_timeline->setRange(0, 0);

    layout->addWidget(m_canvas, 1);
    layout->addWidget(m_timeline);
    setCentralWidget(central);

    auto *fileBar = addToolBar(tr("File"));
    fileBar->addAction(tr("Import"), this, &MainWindow::importMedia);

    auto *viewBar = addToolBar(tr("View"));
    viewBar->addAction(tr("Zoom In"), m_canvas, &PlaybackCanvas::zoomIn);
    viewBar->addAction(tr("Zoom Out"), m_canvas, &PlaybackCanvas::zoomOut);
    viewBar->addAction(tr("Reset View"), m_canvas, &PlaybackCanvas::resetViewTransform);

    auto *drawBar = addToolBar(tr("Draw"));
    drawBar->addAction(tr("Draw None"), [this]() { m_canvas->setDrawMode(PlaybackCanvas::DrawMode::None); });
    drawBar->addAction(tr("Draw Point"), [this]() { m_canvas->setDrawMode(PlaybackCanvas::DrawMode::Point); });
    drawBar->addAction(tr("Draw Line"), [this]() { m_canvas->setDrawMode(PlaybackCanvas::DrawMode::Line); });

    auto *playbackBar = addToolBar(tr("Playback"));
    playbackBar->addAction(tr("Play"), this, &MainWindow::play);
    playbackBar->addAction(tr("Pause"), this, &MainWindow::pause);
    playbackBar->addAction(tr("Stop"), this, &MainWindow::stop);
    playbackBar->addAction(tr("<< 5s"), this, &MainWindow::rewind);
    playbackBar->addAction(tr("5s >>"), this, &MainWindow::fastForward);
    playbackBar->addAction(tr("1x"), this, &MainWindow::normalSpeed);
}

void MainWindow::setupConnections() {
    connect(m_player, &QMediaPlayer::positionChanged, this, &MainWindow::onPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &MainWindow::onDurationChanged);
    connect(m_timeline, &QSlider::sliderMoved, this, &MainWindow::seek);
    connect(m_imageTimer, &QTimer::timeout, this, &MainWindow::nextImageFrame);
}

void MainWindow::importMedia() {
    const QStringList files = QFileDialog::getOpenFileNames(
        this,
        tr("Import video or image sequence"),
        QString(),
        tr("Media Files (*.mp4 *.mov *.avi *.mkv *.wmv *.flv *.webm *.png *.jpg *.jpeg *.bmp *.tif *.tiff)")
    );

    if (files.isEmpty()) {
        return;
    }

    bool allImages = true;
    for (const QString &filePath : files) {
        if (!isImageFile(filePath)) {
            allImages = false;
            break;
        }
    }

    if (allImages) {
        loadImageSequence(files);
        statusBar()->showMessage(tr("Loaded %1 image frames").arg(files.size()));
        return;
    }

    loadVideo(files.first());
    statusBar()->showMessage(tr("Loaded video: %1").arg(files.first()));
}

void MainWindow::play() {
    if (m_currentMediaKind == MediaKind::ImageSequence) {
        m_imageTimer->start();
        return;
    }

    m_player->play();
}

void MainWindow::pause() {
    if (m_currentMediaKind == MediaKind::ImageSequence) {
        m_imageTimer->stop();
        return;
    }

    m_player->pause();
}

void MainWindow::stop() {
    if (m_currentMediaKind == MediaKind::ImageSequence) {
        m_imageTimer->stop();
        m_sequenceIndex = 0;
        showImageAt(m_sequenceIndex);
        return;
    }

    m_player->stop();
}

void MainWindow::fastForward() {
    if (m_currentMediaKind == MediaKind::ImageSequence) {
        m_sequenceIndex = qMin(m_sequenceIndex + 10, m_sequenceFiles.size() - 1);
        showImageAt(m_sequenceIndex);
        return;
    }

    m_player->setPosition(m_player->position() + 5000);
}

void MainWindow::rewind() {
    if (m_currentMediaKind == MediaKind::ImageSequence) {
        m_sequenceIndex = qMax(m_sequenceIndex - 10, 0);
        showImageAt(m_sequenceIndex);
        return;
    }

    m_player->setPosition(qMax<qint64>(m_player->position() - 5000, 0));
}

void MainWindow::normalSpeed() {
    m_player->setPlaybackRate(1.0);
}

void MainWindow::onPositionChanged(qint64 position) {
    if (m_currentMediaKind == MediaKind::Video) {
        m_timeline->setValue(static_cast<int>(position));
    }
}

void MainWindow::onDurationChanged(qint64 duration) {
    if (m_currentMediaKind == MediaKind::Video) {
        m_timeline->setRange(0, static_cast<int>(duration));
    }
}

void MainWindow::seek(int value) {
    if (m_currentMediaKind == MediaKind::ImageSequence) {
        m_sequenceIndex = value;
        showImageAt(m_sequenceIndex);
        return;
    }

    m_player->setPosition(value);
}

void MainWindow::nextImageFrame() {
    if (m_sequenceFiles.isEmpty()) {
        return;
    }

    m_sequenceIndex = (m_sequenceIndex + 1) % m_sequenceFiles.size();
    showImageAt(m_sequenceIndex);
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
}

void MainWindow::loadImageSequence(const QStringList &files) {
    m_currentMediaKind = MediaKind::ImageSequence;

    m_player->stop();
    m_player->setMedia(QMediaContent());

    m_sequenceFiles = files;
    m_sequenceIndex = 0;

    m_timeline->setRange(0, m_sequenceFiles.size() - 1);
    showImageAt(m_sequenceIndex);
}

void MainWindow::showImageAt(int index) {
    if (index < 0 || index >= m_sequenceFiles.size()) {
        return;
    }

    QImage image(m_sequenceFiles.at(index));
    if (!image.isNull()) {
        m_canvas->setImage(image);
        m_timeline->setValue(index);
    }
}

bool MainWindow::isImageFile(const QString &filePath) const {
    QMimeDatabase db;
    const QMimeType type = db.mimeTypeForFile(filePath, QMimeDatabase::MatchContent);
    return type.name().startsWith("image/");
}
