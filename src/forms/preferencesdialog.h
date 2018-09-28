﻿#pragma once

#include "framelesswindow.h"

namespace Ui {
class PreferencesDialog;
}

class PreferencesDialog : public FramelessWindow
{
    Q_OBJECT

signals:
    void play();
    void pause();
    void muteChanged(bool);
    void volumeChanged(unsigned int);
    void urlChanged(const QString &);
    void autostartChanged(bool);
    void about();
    void seekBySlider(qint64);
    void pictureRatioChanged(bool);
    void videoTrackChanged(int);
    void audioTrackChanged(int);
    void subtitleTrackChanged(int);
    void charsetChanged(const QString &);
    void subtitleAutoLoadChanged(bool);
    void subtitleEnabled(bool);

signals:
    void refreshUi();
    void updateVideoSlider(qint64);
    void updateVideoSliderUnit(int);
    void updateVideoSliderRange(qint64);
    void setVideoAreaEnabled(bool);
    void setSeekAreaEnabled(bool);
    void setAudioAreaEnabled(bool);
    void setVolumeAreaEnabled(bool);
    void updateVolumeArea();
    void updateVideoTracks(const QVariantList &);
    void updateAudioTracks(const QVariantList &);
    void updateSubtitleTracks(const QVariantList &);

public:
    explicit PreferencesDialog(QWidget *parent = nullptr);
    ~PreferencesDialog() override;

protected:
    void showEvent(QShowEvent *event) override;
    void changeEvent(QEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void refreshUI();
    void saveSettings();

private:
    Ui::PreferencesDialog *ui;
    bool closing = false, audioAvailable = true;
    unsigned int sliderUnit = 1000;
};