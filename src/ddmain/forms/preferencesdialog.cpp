#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"
#include "settingsmanager.h"
#include "skinsmanager.h"
#include "utils.h"
#include "playerwindow.h"
#include <Win32Utils>

#include <QWinTaskbarButton>
#include <QWinTaskbarProgress>
#include <QFileDialog>
#include <QInputDialog>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QDragEnterEvent>
#include <QUrl>
#include <QMimeDatabase>
#include <QMimeData>
#include <QTextCodec>
#include <QTimer>
#include <QApplication>

void PreferencesDialog::setPlayerWindow(PlayerWindow *player)
{
    playerWindow = player;
}

void PreferencesDialog::updateVideoSlider(qint64 position)
{
    ui->horizontalSlider_video_position->setValue(static_cast<int>(position / sliderUnit));
}

void PreferencesDialog::updateVideoSliderUnit(quint32 unit)
{
    sliderUnit = unit;
}

void PreferencesDialog::updateVideoSliderRange(qint64 duration)
{
    ui->horizontalSlider_video_position->setRange(0, static_cast<int>(duration / sliderUnit));
    int max = ui->horizontalSlider_video_position->maximum();
    auto singleStep = static_cast<int>(max * 0.01);
    ui->horizontalSlider_video_position->setSingleStep(singleStep);
    auto pageStep = static_cast<int>(max * 0.05);
    ui->horizontalSlider_video_position->setPageStep(pageStep);
}

void PreferencesDialog::setSeekAreaEnabled(bool enabled)
{
    ui->horizontalSlider_video_position->setEnabled(enabled);
}

void PreferencesDialog::setAudioAreaEnabled(bool available)
{
    audioAvailable = available;
    ui->groupBox_audio->setEnabled(audioAvailable);
}

void PreferencesDialog::updateVideoTracks(const QVariantList& videoTracks)
{
    if (!videoTracks.isEmpty())
    {
        ui->comboBox_video_track->clear();
        for (auto& track : videoTracks)
        {
            QVariantMap trackData = track.toMap();
            quint32 id = trackData[QStringLiteral("id")].toUInt();
            QString lang = trackData[QStringLiteral("language")].toString();
            QString title = trackData[QStringLiteral("title")].toString();
            QString txt = tr("ID: %0 | Title: %1 | Language: %2")
                    .arg(id).arg(title).arg(lang);
            ui->comboBox_video_track->addItem(txt, id);
        }
    }
    if (ui->comboBox_video_track->count() > 0)
        ui->comboBox_video_track->setEnabled(true);
    else
        ui->comboBox_video_track->setEnabled(false);
}

void PreferencesDialog::updateAudioTracks(const QVariantList& audioTracks, bool add)
{
    if (!audioTracks.isEmpty())
    {
        if (!add)
            ui->comboBox_audio_track->clear();
        for (auto& track : audioTracks)
        {
            QVariantMap trackData = track.toMap();
            quint32 id = trackData[QStringLiteral("id")].toUInt();
            QString lang = trackData[QStringLiteral("language")].toString();
            QString title = trackData[QStringLiteral("title")].toString();
            QString txt = tr("ID: %0 | Title: %1 | Language: %2")
                    .arg(id).arg(title).arg(lang);
            ui->comboBox_audio_track->addItem(txt, id);
        }
    }
    if (ui->comboBox_audio_track->count() > 0)
        ui->comboBox_audio_track->setEnabled(true);
    else
        ui->comboBox_audio_track->setEnabled(false);
}

void PreferencesDialog::updateSubtitleTracks(const QVariantList& subtitleTracks, bool add)
{
    if (!subtitleTracks.isEmpty())
    {
        if (!add)
            ui->comboBox_subtitle_track->clear();
        for (auto& track : subtitleTracks)
        {
            QVariantMap trackData = track.toMap();
            if (!add)
            {
                quint32 id = trackData[QStringLiteral("id")].toUInt();
                QString lang = trackData[QStringLiteral("language")].toString();
                QString title = trackData[QStringLiteral("title")].toString();
                QString txt = tr("ID: %0 | Title: %1 | Language: %2")
                        .arg(id).arg(title).arg(lang);
                ui->comboBox_subtitle_track->addItem(txt, id);
            }
            else
            {
                QString file = trackData[QStringLiteral("file")].toString();
                QString txt = tr("File: %0").arg(QFileInfo(file).fileName());
                ui->comboBox_subtitle_track->addItem(txt, file);
            }
        }
    }
    if (ui->comboBox_subtitle_track->count() > 0)
        ui->comboBox_subtitle_track->setEnabled(true);
    else
        ui->comboBox_subtitle_track->setEnabled(false);
}

void PreferencesDialog::clearAllTracks()
{
    ui->comboBox_video_track->clear();
    ui->comboBox_audio_track->clear();
    ui->comboBox_subtitle_track->clear();
}

void PreferencesDialog::setVolumeToolTip(const QString& text)
{
    ui->checkBox_volume->setToolTip(text);
    ui->horizontalSlider_volume->setToolTip(text);
}

void PreferencesDialog::setVideoPositionText(const QString& text)
{
    ui->label_video_position->setText(text);
}

void PreferencesDialog::setVideoDurationText(const QString& text)
{
    ui->label_video_duration->setText(text);
}

PreferencesDialog::PreferencesDialog(QWidget *parent) : CFramelessWindow(parent)
{
    ui = new Ui::PreferencesDialog();
    ui->setupUi(this);
    setContentsMargins(0, 0, 0, 0);
    setResizeable(true);
    setResizeableAreaWidth(5);
    setTitleBar(ui->widget_windowTitleBar);
    addIgnoreWidget(ui->label_windowTitle);
    initUI();
}

PreferencesDialog::~PreferencesDialog()
{
    delete ui;
    delete taskbarButton;
}

static bool canHandleDrop(const QDragEnterEvent *event)
{
    const QList<QUrl> urls = event->mimeData()->urls();
    if (urls.empty()) return false;
    QMimeDatabase mimeDatabase;
    return SettingsManager::getInstance()->supportedMimeTypes().
            contains(mimeDatabase.mimeTypeForUrl(urls.constFirst()).name());
}

void PreferencesDialog::dragEnterEvent(QDragEnterEvent *event)
{
    CFramelessWindow::dragEnterEvent(event);
    event->setAccepted(canHandleDrop(event));
}

void PreferencesDialog::dropEvent(QDropEvent *event)
{
    CFramelessWindow::dropEvent(event);
    event->accept();
    QUrl url = event->mimeData()->urls().constFirst();
    QString path;
    if (url.isLocalFile())
        path = QDir::toNativeSeparators(url.toLocalFile());
    else
        path = url.url();
    if (ui->comboBox_url->findText(path) < 0)
        ui->comboBox_url->addItem(path, path);
    ui->comboBox_url->setCurrentText(path);
}

void PreferencesDialog::closeEvent(QCloseEvent *event)
{
    CFramelessWindow::closeEvent(event);
    if (ui->comboBox_url->currentText().isEmpty())
        QApplication::quit();
}

void PreferencesDialog::initUI()
{
    taskbarButton = new QWinTaskbarButton();
    taskbarButton->setWindow(windowHandle());
    taskbarProgress = taskbarButton->progress();
    taskbarProgress->setRange(0, 99);
    taskbarProgress->show();
    ui->checkBox_auto_update->setChecked(SettingsManager::getInstance()->getAutoCheckUpdate());
    ui->checkBox_history->setChecked(SettingsManager::getInstance()->isHistoryEnabled());
    ui->spinBox_history->setValue(SettingsManager::getInstance()->getHistoryMax());
    ui->spinBox_history->setEnabled(ui->checkBox_history->isChecked());
    ui->comboBox_video_track->setEnabled(false);
    ui->comboBox_audio_track->setEnabled(false);
    ui->comboBox_subtitle_track->setEnabled(false);
    ui->comboBox_image_quality->addItem(tr("Best"), QStringLiteral("best"));
    ui->comboBox_image_quality->addItem(tr("Fastest"), QStringLiteral("fastest"));
    ui->comboBox_image_quality->addItem(tr("Default"), QStringLiteral("default"));
    ui->comboBox_video_renderer->addItem(QStringLiteral("OpenGLWidget"), QtAV::VideoRendererId_OpenGLWidget);
    ui->comboBox_video_renderer->addItem(QStringLiteral("QGLWidget2"), QtAV::VideoRendererId_GLWidget2);
    ui->comboBox_video_renderer->addItem(QStringLiteral("Widget"), QtAV::VideoRendererId_Widget);
    ui->comboBox_video_renderer->addItem(QStringLiteral("GDI"), QtAV::VideoRendererId_GDI);
    ui->comboBox_video_renderer->addItem(QStringLiteral("Direct2D"), QtAV::VideoRendererId_Direct2D);
    ui->comboBox_skin->addItem(tr("<None>"), QStringLiteral("none"));
    QDir skinDir(QStringLiteral(":/skins"));
    QFileInfoList skinFileList = skinDir.entryInfoList(QDir::Files | QDir::NoSymLinks, QDir::Name);
    if (skinFileList.count() > 0)
        for (auto& skinFile : skinFileList)
            ui->comboBox_skin->addItem(skinFile.completeBaseName(), skinFile.completeBaseName());
    if (ui->comboBox_skin->count() > 1)
        ui->comboBox_skin->setEnabled(true);
    else
        ui->comboBox_skin->setEnabled(false);
    QDir languageDir(QStringLiteral(":/i18n"));
    QFileInfoList languageFileList = languageDir.entryInfoList(QDir::Files | QDir::NoSymLinks, QDir::Name);
    if (languageFileList.count() > 0)
        for (auto& languageFile : languageFileList)
        {
            QString fileName = languageFile.completeBaseName();
            if (!fileName.startsWith(QStringLiteral("dd_"), Qt::CaseInsensitive))
                continue;
            QString lang = fileName.mid(fileName.indexOf(QLatin1Char('_')) + 1);
            lang = lang.replace('-', '_');
            QLocale locale(lang);
            ui->comboBox_language->addItem(locale.nativeLanguageName(), lang);
        }
    if (ui->comboBox_language->count() > 0)
    {
        ui->comboBox_language->insertItem(0, tr("Auto"), QStringLiteral("auto"));
        ui->comboBox_language->insertItem(1, QStringLiteral("American English"), QStringLiteral("en"));
        ui->comboBox_language->setEnabled(true);
    }
    else
    {
        ui->comboBox_language->addItem(tr("<None>"), QStringLiteral("en"));
        ui->comboBox_language->setEnabled(false);
    }
    const QStringList history = SettingsManager::getInstance()->getHistory();
    if (history.count() > 0)
        for (auto& filePath : history)
            ui->comboBox_url->addItem(filePath, filePath);
    if (audioAvailable)
    {
        ui->checkBox_volume->setChecked(!SettingsManager::getInstance()->getMute());
        ui->horizontalSlider_volume->setEnabled(ui->checkBox_volume->isChecked());
        ui->horizontalSlider_volume->setValue(SettingsManager::getInstance()->getVolume());
    }
    ui->checkBox_autoStart->setChecked(Win32Utils::isAutoStartServiceInstalledA("ddassvc"));
    QStringList decoders = SettingsManager::getInstance()->getDecoders();
    ui->checkBox_hwdec_cuda->setChecked(decoders.contains(QStringLiteral("CUDA")));
    ui->checkBox_hwdec_d3d11->setChecked(decoders.contains(QStringLiteral("D3D11")));
    ui->checkBox_hwdec_dxva->setChecked(decoders.contains(QStringLiteral("DXVA")));
    bool hwdecEnabled = (ui->checkBox_hwdec_cuda->isChecked()
                         || ui->checkBox_hwdec_d3d11->isChecked()
                         || ui->checkBox_hwdec_dxva->isChecked())
            && SettingsManager::getInstance()->getHwdec();
    ui->checkBox_hwdec->setChecked(hwdecEnabled);
    ui->checkBox_hwdec_cuda->setEnabled(hwdecEnabled);
    ui->checkBox_hwdec_d3d11->setEnabled(hwdecEnabled);
    ui->checkBox_hwdec_dxva->setEnabled(hwdecEnabled);
    ui->radioButton_ratio_fitDesktop->setChecked(SettingsManager::getInstance()->getFitDesktop());
    ui->radioButton_ratio_videoAspectRatio->setChecked(!ui->radioButton_ratio_fitDesktop->isChecked());
    int i = ui->comboBox_subtitle_charset->findData(SettingsManager::getInstance()->getCharset());
    ui->comboBox_subtitle_charset->setCurrentIndex(i > -1 ? i : 0);
    ui->checkBox_subtitle_autoLoadExternal->setChecked(SettingsManager::getInstance()->getSubtitleAutoLoad());
    ui->checkBox_displaySubtitle->setChecked(SettingsManager::getInstance()->getSubtitle());
    ui->checkBox_audio_autoLoadExternal->setChecked(SettingsManager::getInstance()->getAudioAutoLoad());
    i = ui->comboBox_skin->findData(SettingsManager::getInstance()->getSkin());
    ui->comboBox_skin->setCurrentIndex(i > -1 ? i : 0);
    i = ui->comboBox_language->findData(SettingsManager::getInstance()->getLanguage());
    ui->comboBox_language->setCurrentIndex(i > -1 ? i : 0);
    i = ui->comboBox_video_renderer->findData(SettingsManager::getInstance()->getRenderer());
    ui->comboBox_video_renderer->setCurrentIndex(i > -1 ? i : 0);
    i = ui->comboBox_image_quality->findData(SettingsManager::getInstance()->getImageQuality());
    ui->comboBox_image_quality->setCurrentIndex(i > -1 ? i : 0);
}

void PreferencesDialog::initConnections()
{
    connect(ui->checkBox_auto_update, &QCheckBox::clicked, this, [=]
    {
        bool autoUpdate = ui->checkBox_auto_update->isChecked();
        if (autoUpdate != SettingsManager::getInstance()->getAutoCheckUpdate())
            SettingsManager::getInstance()->setAutoCheckUpdate(autoUpdate);
    });
    connect(ui->checkBox_history, &QCheckBox::clicked, this, [=]
    {
        bool saveHistory = ui->checkBox_history->isChecked();
        if (saveHistory != SettingsManager::getInstance()->isHistoryEnabled())
            SettingsManager::getInstance()->setHistoryEnabled(saveHistory);
        if (ui->spinBox_history->isEnabled() != saveHistory)
            ui->spinBox_history->setEnabled(saveHistory);
        if (saveHistory && ui->spinBox_history->value() < 1)
            ui->spinBox_history->setValue(SettingsManager::getInstance()->getHistoryMax());
    });
    connect(ui->spinBox_history, qOverload<int>(&QSpinBox::valueChanged), this, [=](int value)
    {
        if (value < 1)
        {
            ui->checkBox_history->setChecked(false);
            emit ui->checkBox_history->clicked(false);
        }
        else if (value != SettingsManager::getInstance()->getHistoryMax())
            SettingsManager::getInstance()->setHistoryMax(value);
    });
    connect(ui->horizontalSlider_video_position, &QSlider::valueChanged, taskbarProgress, &QWinTaskbarProgress::setValue);
    connect(ui->horizontalSlider_video_position, &QSlider::rangeChanged, taskbarProgress, &QWinTaskbarProgress::setRange);
    connect(ui->pushButton_audio_open, &QPushButton::clicked, this, [=]
    {
        QString audioPath = QFileDialog::getOpenFileName(nullptr, tr("Please select an audio file"), SettingsManager::getInstance()->lastDir(), tr("Audios (*.mka *.aac *.flac *.mp3 *.wav);;All files (*)"));
        if (!audioPath.isEmpty())
            playerWindow->setAudio(audioPath);
    });
    connect(ui->pushButton_subtitle_open, &QPushButton::clicked, this, [=]
    {
        QString subtitlePath = QFileDialog::getOpenFileName(nullptr, tr("Please select a subtitle file"), SettingsManager::getInstance()->lastDir(), tr("Subtitles (*.ass *.ssa *.srt *.sub);;All files (*)"));
        if (!subtitlePath.isEmpty())
            playerWindow->setSubtitle(subtitlePath);
    });
    connect(ui->horizontalSlider_volume, &QSlider::sliderMoved, this, [=](int value)
    {
        int vol = value;
        if (vol > 99)
            vol = 99;
        if (vol < 0)
            vol = 0;
        if (static_cast<quint32>(vol) != SettingsManager::getInstance()->getVolume())
        {
            SettingsManager::getInstance()->setVolume(static_cast<quint32>(vol));
            playerWindow->setVolume(SettingsManager::getInstance()->getVolume());
        }
    });
    connect(ui->horizontalSlider_video_position, &QSlider::sliderMoved, this, [=](int value)
    {
        playerWindow->seek(static_cast<qint64>(value * sliderUnit));
    });
    connect(ui->pushButton_about, &QPushButton::clicked, this, &PreferencesDialog::about);
    connect(ui->pushButton_minimize, &QPushButton::clicked, this, &PreferencesDialog::showMinimized);
    connect(ui->pushButton_close, &QPushButton::clicked, this, &PreferencesDialog::close);
    connect(ui->checkBox_volume, &QCheckBox::clicked, this, [=]
    {
        ui->horizontalSlider_volume->setEnabled(ui->checkBox_volume->isChecked());
        if (ui->checkBox_volume->isChecked() != !SettingsManager::getInstance()->getMute())
        {
            SettingsManager::getInstance()->setMute(!ui->checkBox_volume->isChecked());
            emit this->muteChanged(SettingsManager::getInstance()->getMute());
            playerWindow->setMute(SettingsManager::getInstance()->getMute());
        }
    });
    connect(ui->pushButton_play, &QPushButton::clicked, this, [=]
    {
        if (ui->comboBox_url->currentText() != SettingsManager::getInstance()->getUrl())
        {
            SettingsManager::getInstance()->setUrl(ui->comboBox_url->currentText());
            playerWindow->setUrl(SettingsManager::getInstance()->getUrl());
            emit this->urlChanged(SettingsManager::getInstance()->getUrl());
        }
        else
            playerWindow->play();
        if (!taskbarProgress->isVisible())
            taskbarProgress->show();
        taskbarProgress->resume();
    });
    connect(ui->pushButton_pause, &QPushButton::clicked, this, [=]
    {
        taskbarProgress->pause();
        playerWindow->pause();
    });
    connect(ui->pushButton_cancel, &QPushButton::clicked, this, &PreferencesDialog::close);
    connect(ui->pushButton_url_browse, &QPushButton::clicked, this, [=]
    {
        QString path = QDir::toNativeSeparators(QFileDialog::getOpenFileName(nullptr, tr("Please select a media file"), SettingsManager::getInstance()->lastDir(), tr("Videos (*.avi *.mp4 *.mkv *.flv);;Audios (*.mp3 *.flac *.ape *.wav);;Pictures (*.bmp *.jpg *.jpeg *.png *.gif);;All files (*)")));
        if (!path.isEmpty())
        {
            if (ui->comboBox_url->findText(path) < 0)
                ui->comboBox_url->addItem(path, path);
            ui->comboBox_url->setCurrentText(path);
        }
    });
    connect(ui->pushButton_url_input, &QPushButton::clicked, this, [=]
    {
        bool ok = false;
        QString input = QInputDialog::getText(nullptr, tr("Please input a valid URL"), tr("URL"), QLineEdit::Normal, QStringLiteral("https://"), &ok);
        if (ok && !input.isEmpty())
        {
            QUrl url(input);
            if (url.isValid())
                if (url.isLocalFile())
                {
                    const QString path = QDir::toNativeSeparators(url.toLocalFile());
                    if (ui->comboBox_url->findText(path) < 0)
                        ui->comboBox_url->addItem(path, path);
                    ui->comboBox_url->setCurrentText(path);
                }
                else
                {
                    if (ui->comboBox_url->findText(url.url()) < 0)
                        ui->comboBox_url->addItem(url.url(), url.url());
                    ui->comboBox_url->setCurrentText(url.url());
                }
            else
                QMessageBox::warning(nullptr, QStringLiteral("Dynamic Desktop"), tr("\"%0\" is not a valid URL.").arg(input));
        }
    });
    connect(ui->checkBox_hwdec, &QCheckBox::clicked, this, [=]
    {
        bool hwdecEnabled = ui->checkBox_hwdec->isChecked();
        ui->checkBox_hwdec_cuda->setEnabled(hwdecEnabled);
        ui->checkBox_hwdec_d3d11->setEnabled(hwdecEnabled);
        ui->checkBox_hwdec_dxva->setEnabled(hwdecEnabled);
        this->setDecoders();
    });
    connect(ui->checkBox_hwdec_cuda, &QCheckBox::clicked, this, &PreferencesDialog::setDecoders);
    connect(ui->checkBox_hwdec_d3d11, &QCheckBox::clicked, this, &PreferencesDialog::setDecoders);
    connect(ui->checkBox_hwdec_dxva, &QCheckBox::clicked, this, &PreferencesDialog::setDecoders);
    ui->comboBox_subtitle_charset->addItem(tr("Auto detect"), QStringLiteral("AutoDetect"));
    ui->comboBox_subtitle_charset->addItem(tr("System"), QStringLiteral("System"));
    for (auto& codec : QTextCodec::availableCodecs())
        ui->comboBox_subtitle_charset->addItem(QString::fromLatin1(codec), QString::fromLatin1(codec));
    connect(ui->comboBox_skin, qOverload<int>(&QComboBox::currentIndexChanged), this, [=](int index)
    {
        Q_UNUSED(index)
        if (ui->comboBox_skin->currentData().toString() != SettingsManager::getInstance()->getSkin())
        {
            SettingsManager::getInstance()->setSkin(ui->comboBox_skin->currentData().toString());
            SkinsManager::getInstance()->setSkin(SettingsManager::getInstance()->getSkin());
        }
    });
    connect(ui->comboBox_language, qOverload<int>(&QComboBox::currentIndexChanged), this, [=](int index)
    {
        Q_UNUSED(index)
        if (ui->comboBox_language->currentData().toString() != SettingsManager::getInstance()->getLanguage())
        {
            SettingsManager::getInstance()->setLanguage(ui->comboBox_language->currentData().toString());
            Utils::installTranslation(SettingsManager::getInstance()->getLanguage(), QStringLiteral("dd"));
            ui->retranslateUi(this);
            emit this->languageChanged();
            QMessageBox::information(nullptr, QStringLiteral("Dynamic Desktop"), tr("Some texts will not refresh their translation until you restart this application."));
        }
    });
    connect(ui->comboBox_video_track, qOverload<int>(&QComboBox::currentIndexChanged), this, [=](int index)
    {
        Q_UNUSED(index)
        playerWindow->setVideoTrack(ui->comboBox_video_track->currentData().toUInt());
    });
    connect(ui->comboBox_audio_track, qOverload<int>(&QComboBox::currentIndexChanged), this, [=](int index)
    {
        Q_UNUSED(index)
        playerWindow->setAudioTrack(ui->comboBox_audio_track->currentData().toUInt());
    });
    connect(ui->comboBox_subtitle_track, qOverload<int>(&QComboBox::currentIndexChanged), this, [=](int index)
    {
        Q_UNUSED(index)
        playerWindow->setSubtitleTrack(ui->comboBox_subtitle_track->currentData());
    });
    connect(ui->comboBox_video_renderer, qOverload<int>(&QComboBox::currentIndexChanged), this, [=](int index)
    {
        Q_UNUSED(index)
        if (ui->comboBox_video_renderer->currentData().toInt() != SettingsManager::getInstance()->getRenderer())
        {
            SettingsManager::getInstance()->setRenderer(ui->comboBox_video_renderer->currentData().toInt());
            playerWindow->setRenderer(SettingsManager::getInstance()->getRenderer());
        }
    });
    connect(ui->comboBox_image_quality, qOverload<int>(&QComboBox::currentIndexChanged), this, [=](int index)
    {
        Q_UNUSED(index)
        if (ui->comboBox_image_quality->currentData().toString() != SettingsManager::getInstance()->getImageQuality())
        {
            SettingsManager::getInstance()->setImageQuality(ui->comboBox_image_quality->currentData().toString());
            playerWindow->setImageQuality(SettingsManager::getInstance()->getImageQuality());
        }
    });
    connect(ui->comboBox_url, &QComboBox::currentTextChanged, this, [=](const QString &text)
    {
        if (text != SettingsManager::getInstance()->getUrl())
        {
            SettingsManager::getInstance()->setUrl(text);
            playerWindow->setUrl(SettingsManager::getInstance()->getUrl());
            emit this->urlChanged(SettingsManager::getInstance()->getUrl());
        }
    });
    connect(ui->checkBox_autoStart, &QCheckBox::clicked, this, [=]
    {
        QString servicePath = QApplication::applicationDirPath() + QStringLiteral("/ddservice");
#ifdef _DEBUG
        servicePath += QStringLiteral("d");
#endif
        servicePath += QStringLiteral(".exe");
        servicePath = QDir::toNativeSeparators(servicePath);
        if (!QFileInfo::exists(servicePath))
        {
            ui->checkBox_autoStart->setChecked(false);
            ui->checkBox_autoStart->setEnabled(false);
            return;
        }
        if (ui->checkBox_autoStart->isChecked() && !Win32Utils::isAutoStartServiceInstalledA("ddassvc"))
            Utils::run(servicePath, QStringList() << QStringLiteral("-i"), true);
        else if (!ui->checkBox_autoStart->isChecked() && Win32Utils::isAutoStartServiceInstalledA("ddassvc"))
            Utils::run(servicePath, QStringList() << QStringLiteral("-u"), true);
        QTimer::singleShot(2500, this, [=]
        {
            bool isAutoStart = Win32Utils::isAutoStartServiceInstalledA("ddassvc");
            if (ui->checkBox_autoStart->isChecked() != isAutoStart)
                ui->checkBox_autoStart->setChecked(isAutoStart);
        });
    });
    connect(ui->radioButton_ratio_fitDesktop, &QRadioButton::clicked, this, &PreferencesDialog::setRatio);
    connect(ui->radioButton_ratio_videoAspectRatio, &QRadioButton::clicked, this, &PreferencesDialog::setRatio);
    connect(ui->comboBox_subtitle_charset, qOverload<int>(&QComboBox::currentIndexChanged), this, [=](int index)
    {
        Q_UNUSED(index)
        if (ui->comboBox_subtitle_charset->currentData().toString() != SettingsManager::getInstance()->getCharset())
        {
            SettingsManager::getInstance()->setCharset(ui->comboBox_subtitle_charset->currentData().toString());
            playerWindow->setCharset(SettingsManager::getInstance()->getCharset());
        }
    });
    connect(ui->checkBox_subtitle_autoLoadExternal, &QCheckBox::clicked, this, [=]
    {
        if (ui->checkBox_subtitle_autoLoadExternal->isChecked() != SettingsManager::getInstance()->getSubtitleAutoLoad())
        {
            SettingsManager::getInstance()->setSubtitleAutoLoad(ui->checkBox_subtitle_autoLoadExternal->isChecked());
            playerWindow->setSubtitleAutoLoad(SettingsManager::getInstance()->getSubtitleAutoLoad());
        }
    });
    connect(ui->checkBox_displaySubtitle, &QCheckBox::clicked, this, [=]
    {
        if (ui->checkBox_displaySubtitle->isChecked() != SettingsManager::getInstance()->getSubtitle())
        {
            SettingsManager::getInstance()->setSubtitle(ui->checkBox_displaySubtitle->isChecked());
            playerWindow->setSubtitleEnabled(SettingsManager::getInstance()->getSubtitle());
        }
    });
    connect(this, &PreferencesDialog::setMute, this, [=](bool mute)
    {
        ui->checkBox_volume->setChecked(!mute);
        emit ui->checkBox_volume->clicked(!mute);
    });
}

void PreferencesDialog::setDecoders()
{
    QStringList decoders;
    if (ui->checkBox_hwdec_cuda->isChecked())
        decoders << QStringLiteral("CUDA");
    if (ui->checkBox_hwdec_d3d11->isChecked())
        decoders << QStringLiteral("D3D11");
    if (ui->checkBox_hwdec_dxva->isChecked())
        decoders << QStringLiteral("DXVA");
    if (decoders.isEmpty())
        ui->checkBox_hwdec->setChecked(false);
    if (ui->checkBox_hwdec->isChecked() != SettingsManager::getInstance()->getHwdec())
        SettingsManager::getInstance()->setHwdec(ui->checkBox_hwdec->isChecked());
    decoders << QStringLiteral("FFmpeg");
    if (decoders != SettingsManager::getInstance()->getDecoders())
        SettingsManager::getInstance()->setDecoders(decoders);
    if (isVisible() && isActiveWindow())
        QMessageBox::information(nullptr, QStringLiteral("Dynamic Desktop"), tr("Reopen this video or play another video to experience it.\nMake sure this application runs in your GPU's Optimus mode."));
}

void PreferencesDialog::setRatio()
{
    if (ui->radioButton_ratio_fitDesktop->isChecked() != SettingsManager::getInstance()->getFitDesktop())
    {
        SettingsManager::getInstance()->setFitDesktop(ui->radioButton_ratio_fitDesktop->isChecked());
        playerWindow->setImageRatio(SettingsManager::getInstance()->getFitDesktop());
    }
}

void PreferencesDialog::togglePlayPause()
{
    if (!playerWindow->isMediaLoaded())
        return;
    if (playerWindow->isMediaPlaying())
        emit ui->pushButton_pause->clicked();
    else if (playerWindow->isMediaPaused())
        emit ui->pushButton_play->clicked();
}
