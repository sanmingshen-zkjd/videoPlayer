#pragma once

#include "playbackcanvas.h"

#include <QMainWindow>
#include <QStringList>

class QMediaPlayer;
class QSlider;
class QTimer;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void importMedia();
    void play();
    void pause();
    void stop();
    void fastForward();
    void rewind();
    void normalSpeed();
    void onPositionChanged(qint64 position);
    void onDurationChanged(qint64 duration);
    void seek(int value);
    void nextImageFrame();

private:
    enum class MediaKind {
        None,
        Video,
        ImageSequence
    };

    PlaybackCanvas *m_canvas;

    QMediaPlayer *m_player;
    QTimer *m_imageTimer;

    QSlider *m_timeline;

    QStringList m_sequenceFiles;
    int m_sequenceIndex;
    MediaKind m_currentMediaKind;

    void setupUi();
    void setupConnections();

    void loadVideo(const QString &filePath);
    void loadImageSequence(const QStringList &files);
    void showImageAt(int index);
    bool isImageFile(const QString &filePath) const;
};
