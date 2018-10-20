#pragma once

#include <QWidget>
#include <QVariant>

QT_BEGIN_NAMESPACE
QT_FORWARD_DECLARE_CLASS(QVBoxLayout)
QT_FORWARD_DECLARE_CLASS(QWinTaskbarButton)
QT_FORWARD_DECLARE_CLASS(QWinTaskbarProgress)
QT_END_NAMESPACE

namespace QtAV
{
    QT_FORWARD_DECLARE_CLASS(AVPlayer)
    QT_FORWARD_DECLARE_CLASS(SubtitleFilter)
    QT_FORWARD_DECLARE_CLASS(VideoRenderer)
}

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

signals:
    void sendCommand(QPair<QString, QVariant>);

public slots:
    void parseCommand(const QPair<QString, QVariant>& command);

public slots:
    void controllerEcho(const QVariant& param);
    void setVolume(const QVariant& param);
    void setMute(const QVariant& param);
    void seek(const QVariant& param);
    void setVideoTrack(const QVariant& param);
    void setAudioTrack(const QVariant& param);
    void setSubtitleTrack(const QVariant& param);
    void setSubtitle(const QVariant& param);
    void setAudio(const QVariant& param);
    void setCharset(const QVariant& param);
    void setSubtitleAutoLoad(const QVariant& param);
    void setSubtitleEnabled(const QVariant& param);
    void play(const QVariant& param);
    void pause(const QVariant& param);
    void stop(const QVariant& param);
    void setUrl(const QVariant& param);
    bool setRenderer(const QVariant& param);
    void setImageQuality(const QVariant& param);
    void setImageRatio(const QVariant& param);

private slots:
    void initUI();
    void initPlayer();
    void initConnections();
    void initAudio();
    void onStartPlay();

private:
    QtAV::AVPlayer *player = nullptr;
    QtAV::VideoRenderer *renderer = nullptr;
    QtAV::SubtitleFilter *subtitle = nullptr;
    QVBoxLayout *mainLayout = nullptr;
    QWinTaskbarButton *taskbarButton = nullptr;
    QWinTaskbarProgress *taskbarProgress = nullptr;
};
