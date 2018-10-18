#include <SettingsManager>
#include <SkinsManager>
#include <Utils>
#include <IPCServer>
#include "forms/preferencesdialog.h"
#include "forms/aboutdialog.h"
#include "../common.h"

#include <Windows.h>

#include <QApplication>
#include <QMessageBox>
#include <QTranslator>
#include <QLocale>
#ifndef BUILD_DD_STATIC
#include <QLibraryInfo>
#endif
#include <QSysInfo>
#include <QVersionNumber>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QAction>
#include <QThread>

static HANDLE mutex = nullptr;

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Dynamic Desktop Launcher"));
    QApplication::setApplicationDisplayName(QStringLiteral("Dynamic Desktop Launcher"));
    QApplication::setApplicationVersion(QStringLiteral(DD_VERSION));
    QApplication::setOrganizationName(QStringLiteral("wangwenx190"));
    QApplication::setOrganizationDomain(QStringLiteral("wangwenx190.github.io"));
#ifndef _DEBUG
    const QStringList arguments = QApplication::arguments();
    if (!arguments.contains(QStringLiteral("-?"))
            && !arguments.contains(QStringLiteral("-h"), Qt::CaseInsensitive)
            && !arguments.contains(QStringLiteral("--help"), Qt::CaseInsensitive)
            && !arguments.contains(QStringLiteral("-v"), Qt::CaseInsensitive)
            && !arguments.contains(QStringLiteral("--version"), Qt::CaseInsensitive)
            && !QApplication::arguments().contains(QStringLiteral("--launch"), Qt::CaseInsensitive))
        if (Utils::checkUpdate())
            return 0;
#endif
#ifdef BUILD_DD_STATIC
    QString qmDir = QStringLiteral(":/i18n");
#else
    QString qmDir = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
#endif
    QString language = SettingsManager::getInstance()->getLanguage();
    QTranslator translator;
    if (language == QStringLiteral("auto"))
    {
        if (translator.load(QLocale(), QStringLiteral("launcher"), QStringLiteral("_"), qmDir))
            QApplication::installTranslator(&translator);
    }
    else
    {
        language = QStringLiteral("launcher_%0.qm").arg(language);
        if (translator.load(language, qmDir))
            QApplication::installTranslator(&translator);
    }
    int suffixIndex;
    QVersionNumber currentVersion = QVersionNumber::fromString(QSysInfo::kernelVersion(), &suffixIndex);
    QVersionNumber win7Version(6, 1, 7600);
    if (currentVersion < win7Version)
    {
        QMessageBox::critical(nullptr, QStringLiteral("Dynamic Desktop"), QObject::tr("This application only supports Windows 7 and newer."));
        Utils::Exit(-1, true, mutex);
    }
    mutex = CreateMutex(nullptr, FALSE, TEXT("wangwenx190.DynamicDesktop.Launcher.1000.AppMutex"));
    if ((mutex != nullptr) && (GetLastError() == ERROR_ALREADY_EXISTS))
    {
        QMessageBox::critical(nullptr, QStringLiteral("Dynamic Desktop"), QObject::tr("There is another instance running. Please do not run twice."));
        ReleaseMutex(mutex);
        return 0;
    }
    app.setQuitOnLastWindowClosed(false);
    QCommandLineParser parser;
    parser.setApplicationDescription(QObject::tr("A tool that make your desktop alive."));
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption windowModeOption(QStringLiteral("window"),
                                        QApplication::translate("main", "Show a normal window instead of placing it under the desktop icons."));
    parser.addOption(windowModeOption);
    QCommandLineOption skinOption(QStringLiteral("skin"),
                                  QApplication::translate("main", "Set skin. The value is the file name of the skin file, excluding the file extension. If it's not under the \"skins\" folder, please give the absolute path of the file."),
                                  QApplication::translate("main", "Skin file name"));
    parser.addOption(skinOption);
    QCommandLineOption urlOption(QStringLiteral("url"),
                                 QApplication::translate("main", "Play the given url. It can be a local file or a valid web url. Default is empty."),
                                 QApplication::translate("main", "url"));
    parser.addOption(urlOption);
    /*QCommandLineOption keepRatioOption(QStringLiteral("keepratio"),
                                        QApplication::translate("main", "Make the output image keep original video aspect ratio instead of fitting the whole renderer window."));
    parser.addOption(keepRatioOption);*/
    QCommandLineOption imageQualityOption(QStringLiteral("quality"),
                                          QApplication::translate("main", "Set the quality of the output image. It can be default/best/fastest. Default is best. Case insensitive."),
                                          QApplication::translate("main", "Image quality"));
    parser.addOption(imageQualityOption);
    QCommandLineOption rendererOption(QStringLiteral("renderer"),
                                      QApplication::translate("main", "Set rendering engine. It can be opengl/gl/qt/gdi/d2d. Default is gl. Case insensitive."),
                                      QApplication::translate("main", "renderer"));
    parser.addOption(rendererOption);
    QCommandLineOption volumeOption(QStringLiteral("volume"),
                                    QApplication::translate("main", "Set volume. It must be a positive integer between 0 and 99. Default is 9."),
                                    QApplication::translate("main", "volume"));
    parser.addOption(volumeOption);
    QCommandLineOption launchOption(QStringLiteral("launch"),
                                    QApplication::translate("main", "Skip checking for updates, launch directly."));
    parser.addOption(launchOption);
    parser.process(app);
    QString skinOptionValue = parser.value(skinOption);
    if (!skinOptionValue.isEmpty())
        if (skinOptionValue != SettingsManager::getInstance()->getSkin())
            SettingsManager::getInstance()->setSkin(skinOptionValue);
    QString urlOptionValue = parser.value(urlOption);
    if (!urlOptionValue.isEmpty())
        if (urlOptionValue != SettingsManager::getInstance()->getUrl())
            SettingsManager::getInstance()->setUrl(urlOptionValue);
    /*bool keepRatioOptionValue = parser.isSet(keepRatioOption);
    if (keepRatioOptionValue != !SettingsManager::getInstance()->getFitDesktop())
        SettingsManager::getInstance()->setFitDesktop(!keepRatioOptionValue);*/
    QString imageQualityOptionValue = parser.value(imageQualityOption).toLower();
    if (!imageQualityOptionValue.isEmpty())
        if (((imageQualityOptionValue == QStringLiteral("default")) ||
             (imageQualityOptionValue == QStringLiteral("best")) ||
             (imageQualityOptionValue == QStringLiteral("fastest"))) &&
                (imageQualityOptionValue != SettingsManager::getInstance()->getImageQuality()))
            SettingsManager::getInstance()->setImageQuality(imageQualityOptionValue);
    QString rendererOptionValue = parser.value(rendererOption).toLower();
    if (!rendererOptionValue.isEmpty())
        if ((rendererOptionValue == QStringLiteral("opengl")) &&
                (SettingsManager::getInstance()->getRenderer() != QtAV_VId_OpenGLWidget))
            SettingsManager::getInstance()->setRenderer(QtAV_VId_OpenGLWidget);
        else if ((rendererOptionValue == QStringLiteral("gl")) &&
                 (SettingsManager::getInstance()->getRenderer() != QtAV_VId_GLWidget2))
            SettingsManager::getInstance()->setRenderer(QtAV_VId_GLWidget2);
        else if ((rendererOptionValue == QStringLiteral("qt")) &&
                 (SettingsManager::getInstance()->getRenderer() != QtAV_VId_Widget))
            SettingsManager::getInstance()->setRenderer(QtAV_VId_Widget);
        else if ((rendererOptionValue == QStringLiteral("gdi")) &&
                 (SettingsManager::getInstance()->getRenderer() != QtAV_VId_GDI))
            SettingsManager::getInstance()->setRenderer(QtAV_VId_GDI);
        else if ((rendererOptionValue == QStringLiteral("d2d")) &&
                 (SettingsManager::getInstance()->getRenderer() != QtAV_VId_Direct2D))
            SettingsManager::getInstance()->setRenderer(QtAV_VId_Direct2D);
    QString volumeOptionValue = parser.value(volumeOption);
    if (!volumeOptionValue.isEmpty())
    {
        int volumeOptionValueInt = volumeOptionValue.toInt();
        if (volumeOptionValueInt < 0)
            volumeOptionValueInt = 0;
        if (volumeOptionValueInt > 99)
            volumeOptionValueInt = 99;
        if (static_cast<quint32>(volumeOptionValueInt) != SettingsManager::getInstance()->getVolume())
            SettingsManager::getInstance()->setVolume(static_cast<quint32>(volumeOptionValueInt));
    }
    SkinsManager::getInstance()->setSkin(SettingsManager::getInstance()->getSkin());
    PreferencesDialog preferencesDialog;
    AboutDialog aboutDialog;
    QMenu trayMenu;
    QSystemTrayIcon trayIcon;
    trayIcon.setIcon(QIcon(QStringLiteral(":/icons/color_palette.ico")));
    trayIcon.setToolTip(QStringLiteral("Dynamic Desktop"));
    trayIcon.setContextMenu(&trayMenu);
    trayIcon.show();
    QAction *showPreferencesDialogAction = trayMenu.addAction(QObject::tr("Preferences"), [=, &preferencesDialog]
    {
        if (preferencesDialog.isHidden())
        {
            Utils::moveToCenter(&preferencesDialog);
            preferencesDialog.show();
        }
        if (!preferencesDialog.isActiveWindow())
            preferencesDialog.setWindowState(preferencesDialog.windowState() & ~Qt::WindowMinimized);
        if (!preferencesDialog.isActiveWindow())
        {
            preferencesDialog.raise();
            preferencesDialog.activateWindow();
        }
    });
    trayMenu.addSeparator();
    trayMenu.addAction(QObject::tr("Play"), [=, &preferencesDialog]
    {
        emit preferencesDialog.sendCommand(qMakePair(QStringLiteral("play"), QVariant()));
    });
    trayMenu.addAction(QObject::tr("Pause"), [=, &preferencesDialog]
    {
        emit preferencesDialog.sendCommand(qMakePair(QStringLiteral("pause"), QVariant()));
    });
    QAction *muteAction = trayMenu.addAction(QObject::tr("Mute"));
    muteAction->setCheckable(true);
    muteAction->setChecked(SettingsManager::getInstance()->getMute());
    QObject::connect(muteAction, &QAction::triggered, [=, &preferencesDialog]
    {
        emit preferencesDialog.setMute(muteAction->isChecked());
    });
    trayMenu.addSeparator();
    QAction *aboutAction = trayMenu.addAction(QObject::tr("About"), [=, &aboutDialog]
    {
        if (aboutDialog.isHidden())
        {
            Utils::moveToCenter(&aboutDialog);
            aboutDialog.show();
        }
        if (!aboutDialog.isActiveWindow())
            aboutDialog.setWindowState(aboutDialog.windowState() & ~Qt::WindowMinimized);
        if (!aboutDialog.isActiveWindow())
        {
            aboutDialog.raise();
            aboutDialog.activateWindow();
        }
    });
    QObject::connect(&preferencesDialog, &PreferencesDialog::about, [=]
    {
        emit aboutAction->triggered();
    });
    QString playerFileName = QStringLiteral("player");
#ifdef _DEBUG
    playerFileName += QStringLiteral("d");
#endif
    playerFileName += QStringLiteral(".exe");
    QObject::connect(&preferencesDialog, &PreferencesDialog::requestQuit, [=, &preferencesDialog, &trayIcon](bool fromPlayer)
    {
        qApp->closeAllWindows();
        trayIcon.hide();
        if (!fromPlayer)
        {
            emit preferencesDialog.sendCommand(qMakePair(QStringLiteral("quit"), QVariant()));
            QThread::sleep(2);
            // Make sure the player process is terminated
            Utils::killProcess(playerFileName);
        }
        qApp->quit();
    });
    trayMenu.addAction(QObject::tr("Exit"), [=, &preferencesDialog]
    {
        emit preferencesDialog.requestQuit(false);
    });
    QObject::connect(&trayIcon, &QSystemTrayIcon::activated, [=](QSystemTrayIcon::ActivationReason reason)
    {
        if (reason != QSystemTrayIcon::Context)
            emit showPreferencesDialogAction->triggered();
    });
    QObject::connect(&preferencesDialog, &PreferencesDialog::muteChanged, [=, &preferencesDialog](bool mute)
    {
        muteAction->setChecked(mute);
    });
    QString playerPath = QApplication::applicationDirPath() + QLatin1Char('/') + playerFileName;
    QStringList playerStartupArguments{ QStringLiteral("--runfromlauncher") };
    if (parser.isSet(windowModeOption))
        playerStartupArguments << QStringLiteral("--window");
    IPCServer ipcServer;
    QObject::connect(&ipcServer, &IPCServer::clientMessage, &preferencesDialog, &PreferencesDialog::parseCommand);
    QObject::connect(&preferencesDialog, &PreferencesDialog::sendCommand, &ipcServer, &IPCServer::serverMessage);
    if (!Utils::run(playerPath, playerStartupArguments))
    {
        QMessageBox::critical(nullptr, QStringLiteral("Dynamic Desktop"), QObject::tr("Cannot start the core module. Application aborting."));
        Utils::Exit(-1, true, mutex);
    }
    if (SettingsManager::getInstance()->getUrl().isEmpty())
    {
        Utils::moveToCenter(&preferencesDialog);
        preferencesDialog.show();
    }
    return Utils::Exit(QApplication::exec(), false, mutex);
}
