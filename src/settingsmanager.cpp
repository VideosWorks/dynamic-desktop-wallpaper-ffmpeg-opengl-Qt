﻿#include "settingsmanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QUrl>
#include <QFileInfo>
#include <QMimeDatabase>

SettingsManager *SettingsManager::getInstance()
{
    static SettingsManager settingsManager;
    return &settingsManager;
}

void SettingsManager::unregAutostart()
{
    const QString key = QStringLiteral("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
    QSettings set(key, QSettings::NativeFormat);
    if (set.contains(QStringLiteral("Dynamic Desktop")))
        set.remove(QStringLiteral("Dynamic Desktop"));
}

bool SettingsManager::regAutostart()
{
    unregAutostart();
    const QString key = QStringLiteral("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
    QSettings set(key, QSettings::NativeFormat);
    if (set.status() != QSettings::NoError)
        return false;
    QString value = QLatin1Char('"') + QCoreApplication::applicationFilePath() + QLatin1Char('"');
    set.setValue(QStringLiteral("Dynamic Desktop"), QDir::toNativeSeparators(value));
    return true;
}

QStringList SettingsManager::defaultDecoders() const
{
    return QStringList()
            << QStringLiteral("CUDA")
            << QStringLiteral("D3D11")
            << QStringLiteral("DXVA")
            << QStringLiteral("FFmpeg");
}

QStringList SettingsManager::supportedMimeTypes()
{
    return QStringList()
            << QStringLiteral("audio/ac3")
            << QStringLiteral("audio/eac3")
            << QStringLiteral("audio/vnd.dolby.mlp")
            << QStringLiteral("audio/vnd.dts")
            << QStringLiteral("audio/vnd.dts.hd")
            << QStringLiteral("audio/wav")
            << QStringLiteral("audio/aiff")
            << QStringLiteral("audio/amr")
            << QStringLiteral("audio/amr-wb")
            << QStringLiteral("audio/basic")
            << QStringLiteral("audio/x-ape")
            << QStringLiteral("audio/x-wavpack")
            << QStringLiteral("audio/x-shorten")
            << QStringLiteral("video/vnd.dlna.mpeg-tts")
            << QStringLiteral("audio/vnd.dlna.adts")
            << QStringLiteral("audio/mpeg")
            << QStringLiteral("video/mpeg")
            << QStringLiteral("video/dvd")
            << QStringLiteral("video/mp4")
            << QStringLiteral("audio/mp4")
            << QStringLiteral("audio/aac")
            << QStringLiteral("audio/flac")
            << QStringLiteral("audio/ogg")
            << QStringLiteral("video/ogg")
            << QStringLiteral("application/ogg")
            << QStringLiteral("video/x-matroska")
            << QStringLiteral("audio/x-matroska")
            << QStringLiteral("video/webm")
            << QStringLiteral("audio/webm")
            << QStringLiteral("video/avi")
            << QStringLiteral("video/x-msvideo")
            << QStringLiteral("video/flc")
            << QStringLiteral("application/gxf")
            << QStringLiteral("application/mxf")
            << QStringLiteral("audio/x-ms-wma")
            << QStringLiteral("video/x-ms-wm")
            << QStringLiteral("video/x-ms-wmv")
            << QStringLiteral("video/x-ms-asf")
            << QStringLiteral("video/x-flv")
            << QStringLiteral("video/mp4")
            << QStringLiteral("audio/mp4")
            << QStringLiteral("video/quicktime")
            << QStringLiteral("application/vnd.rn-realmedia")
            << QStringLiteral("application/vnd.rn-realmedia-vbr")
            << QStringLiteral("audio/vnd.rn-realaudio")
            << QStringLiteral("audio/3gpp")
            << QStringLiteral("audio/3gpp2")
            << QStringLiteral("video/3gpp")
            << QStringLiteral("video/3gpp2")
            << QStringLiteral("audio/x-mpegurl")
            << QStringLiteral("audio/x-scpls");
}

QStringList SettingsManager::supportedSuffixes()
{
    QStringList mSupportedSuffixes;
    QMimeDatabase mMimeDatabase;
    const QStringList mSupportedMimeTypes = supportedMimeTypes();
    for (auto& mFileType : mSupportedMimeTypes)
    {
        const QStringList mSuffixes = mMimeDatabase.mimeTypeForName(mFileType).suffixes();
        for (auto mSuffix : mSuffixes)
        {
            mSuffix.prepend(QStringLiteral("*."));
            mSupportedSuffixes << mSuffix;
        }
    }
    return mSupportedSuffixes;
}

QString SettingsManager::getUrl() const
{
    QString path = settings->value(QStringLiteral("dd/url"), QString()).toString();
    if (QFileInfo(path).isDir())
        return QString();
    if (!QFileInfo::exists(path))
    {
        QUrl url(path);
        if (!url.isValid())
            return QString();
        if (url.isLocalFile())
            return url.toLocalFile();
        return QUrl::fromPercentEncoding(url.toEncoded());
    }
    return path;
}

bool SettingsManager::getMute() const
{
    return settings->value(QStringLiteral("dd/mute"), false).toBool();
}

unsigned int SettingsManager::getVolume() const
{
    int vol = settings->value(QStringLiteral("dd/volume"), 9).toInt();
    if (vol < 0)
        vol = 0;
    if (vol > 99)
        vol = 99;
    return static_cast<unsigned int>(vol);
}

bool SettingsManager::getAutostart() const
{
    return settings->value(QStringLiteral("dd/autostart"), false).toBool();
}

bool SettingsManager::getHwdec() const
{
    return settings->value(QStringLiteral("dd/hwdec"), true).toBool();
}

QStringList SettingsManager::getDecoders() const
{
    return settings->value(QStringLiteral("dd/decoders"), defaultDecoders()).toStringList();
}

bool SettingsManager::getLocalize() const
{
    return settings->value(QStringLiteral("dd/localize"), true).toBool();
}

bool SettingsManager::getFitDesktop() const
{
    return settings->value(QStringLiteral("dd/fit"), true).toBool();
}

void SettingsManager::setUrl(const QString &url)
{
    settings->setValue(QStringLiteral("dd/url"), url);
}

void SettingsManager::setMute(bool mute)
{
    settings->setValue(QStringLiteral("dd/mute"), mute);
}

void SettingsManager::setVolume(unsigned int volume)
{
    unsigned int vol = volume;
    if (vol > 99)
        vol = 99;
    settings->setValue(QStringLiteral("dd/volume"), vol);
}

void SettingsManager::setAutostart(bool enable)
{
    settings->setValue(QStringLiteral("dd/autostart"), enable);
}

void SettingsManager::setHwdec(bool enable)
{
    settings->setValue(QStringLiteral("dd/hwdec"), enable);
}

void SettingsManager::setDecoders(const QStringList &decoders)
{
    settings->setValue(QStringLiteral("dd/decoders"), decoders);
}

void SettingsManager::setLocalize(bool enable)
{
    settings->setValue(QStringLiteral("dd/localize"), enable);
}

void SettingsManager::setFitDesktop(bool fit)
{
    settings->setValue(QStringLiteral("dd/fit"), fit);
}

SettingsManager::SettingsManager()
{
    QString iniPath = QCoreApplication::applicationFilePath();
    if (iniPath.endsWith(QStringLiteral(".exe"), Qt::CaseInsensitive))
        iniPath = iniPath.remove(iniPath.lastIndexOf(QLatin1Char('.')), 4);
    iniPath += QStringLiteral(".ini");
    settings = new QSettings(iniPath, QSettings::IniFormat);
}

SettingsManager::~SettingsManager()
{
    delete settings;
}
