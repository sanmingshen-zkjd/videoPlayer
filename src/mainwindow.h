#pragma once

#include "playbackcanvas.h"

#include <QMainWindow>
#include <QStringList>

class QComboBox;
class QDialog;
class QEvent;
class QMediaPlayer;
class QSlider;
class QTimer;
class QToolBar;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void importMedia();
    void play();
    void pause();
    void stop();
    void fastForward();
    void rewind();
    void onPositionChanged(qint64 position);
    void onDurationChanged(qint64 duration);
    void seek(int value);
    void nextImageFrame();
    void openImageAdjustDialog();
    void onPlaybackRateChanged(const QString &text);
    void onPointDrawn(const QPointF &scenePos, const QImage &aroundImage);

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
    QComboBox *m_rateCombo;
    QToolBar *m_quickToolBar;

    QStringList m_sequenceFiles;
    int m_sequenceIndex;
    MediaKind m_currentMediaKind;

    QDialog *m_adjustDialog;
    int m_brightness;
    int m_contrast;
    double m_playbackRate;

    void setupUi();
    void setupConnections();
    void buildMenuBar();
    void buildMainToolBar();
    void buildQuickToolBar();

    void loadVideo(const QString &filePath);
    void loadImageSequence(const QStringList &files);
    void showImageAt(int index);
    bool isImageFile(const QString &filePath) const;
    void applyImageAdjustments();
    void setPlaybackRate(double rate);
};
