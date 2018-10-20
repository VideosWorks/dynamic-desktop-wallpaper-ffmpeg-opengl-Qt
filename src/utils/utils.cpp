#include "utils.h"

#include <QMutex>
#include <QApplication>
#include <QTextStream>
#include <QDir>
#include <QDesktopWidget>
#include <QFileInfo>
#include <QProcess>
#include <QWidget>

#include <wtsapi32.h>
#include <userenv.h>
#include <tlhelp32.h>
#include <tchar.h>

namespace Utils
{

void fileLogger(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    static QMutex mutex;
    mutex.lock();
    QString msgType;
    switch (type)
    {
    case QtDebugMsg:
        msgType = QStringLiteral("DEBUG");
        break;
    case QtInfoMsg:
        msgType = QStringLiteral("INFORMATION");
        break;
    case QtWarningMsg:
        msgType = QStringLiteral("WARNING");
        break;
    case QtCriticalMsg:
        msgType = QStringLiteral("CRITICAL");
        break;
    case QtFatalMsg:
        msgType = QStringLiteral("FATAL");
        break;
        /*case QtSystemMsg:
        msgType = QStringLiteral("SYSTEM");
        break;*/
    default:
        msgType = QStringLiteral("DEBUG");
        break;
    }
    QString messageStr = QStringLiteral("%0\t%1\t%2\t%3\t%4")
            .arg(msgType).arg(msg).arg(context.file).arg(context.line).arg(context.function);
    QString logPath = QCoreApplication::applicationDirPath();
    logPath += QStringLiteral("/debug.log");
    QFile file(logPath);
    if (file.open(QFile::WriteOnly | QFile::Append | QFile::Text))
    {
        QTextStream ts(&file);
        ts << messageStr << QLatin1Char('\n');
        file.close();
    }
    mutex.unlock();
}

QStringList externalFilesToLoad(const QFileInfo &originalMediaFile, const QString &fileType)
{
    if (!originalMediaFile.exists() || originalMediaFile.isDir() || fileType.isEmpty())
        return QStringList();
    QDir subDir(originalMediaFile.absoluteDir());
    QFileInfoList fileList = subDir.entryInfoList(QDir::Files | QDir::NoSymLinks, QDir::Name);
    if (fileList.count() < 2)
        return QStringList();
    const QString fileBaseName = originalMediaFile.baseName().toLower();
    QStringList newFileList;
    for (auto& fi : fileList)
    {
        if (fi.absoluteFilePath() == originalMediaFile.absoluteFilePath())
            continue;
        const QString newBaseName = fi.baseName().toLower();
        if (newBaseName == fileBaseName)
            if (fileType.toLower() == QLatin1String("sub"))
            {
                if (fi.suffix().toLower() == QLatin1String("ass")
                        || fi.suffix().toLower() == QLatin1String("ssa")
                        || fi.suffix().toLower() == QLatin1String("srt")
                        || fi.suffix().toLower() == QLatin1String("sub"))
                    newFileList.append(QDir::toNativeSeparators(fi.absoluteFilePath()));
            }
            else if (fileType.toLower() == QLatin1String("audio"))
                if (fi.suffix().toLower() == QLatin1String("mka"))
                    newFileList.append(QDir::toNativeSeparators(fi.absoluteFilePath()));
    }
    return newFileList;
}

void moveToCenter(QObject *window)
{
    if (!window)
        return;
    auto win = qobject_cast<QWidget *>(window);
    quint32 screenWidth = QApplication::desktop()->screenGeometry(win).width();
    quint32 screenHeight = QApplication::desktop()->screenGeometry(win).height();
    quint32 windowWidth = win->width();
    quint32 windowHeight = win->height();
    quint32 newX = (screenWidth - windowWidth) / 2;
    quint32 newY = (screenHeight - windowHeight) / 2;
    win->move(newX, newY);
}

bool adminRun(const QString &path, const QString &params)
{
    if (path.isEmpty())
        return false;
    if (!QFileInfo::exists(path))
        return false;
    SHELLEXECUTEINFO execInfo{ sizeof(SHELLEXECUTEINFO) };
    execInfo.lpVerb = TEXT("runas");
    execInfo.lpFile = reinterpret_cast<const wchar_t *>(QDir::toNativeSeparators(path).utf16());
    execInfo.nShow = SW_HIDE;
    execInfo.lpParameters = params.isEmpty() ? nullptr : reinterpret_cast<const wchar_t *>(params.utf16());
    return ShellExecuteEx(&execInfo);
}

bool checkUpdate(bool autoUpdate)
{
    QStringList arguments = QCoreApplication::arguments();
    arguments.takeFirst();
    if (autoUpdate)
        arguments << QStringLiteral("--auto-update");
    arguments.insert(0, QStringLiteral("--updater"));
    return QProcess::startDetached(QDir::toNativeSeparators(QCoreApplication::applicationFilePath()), arguments, QDir::toNativeSeparators(QCoreApplication::applicationDirPath()));
}

bool launchSession1Process(const QString &path, const QString &params)
{
    if (path.isEmpty())
        return false;
    if (!QFileInfo::exists(path))
        return false;
    const QString dir = QDir::toNativeSeparators(QFileInfo(path).canonicalPath());
    STARTUPINFO si = { 0 };
    PROCESS_INFORMATION pi = { nullptr };
    si.cb = sizeof(si);
    DWORD dwSessionID = WTSGetActiveConsoleSessionId();
    HANDLE hToken = nullptr;
    if (WTSQueryUserToken(dwSessionID, &hToken) == FALSE)
        return false;
    HANDLE hDuplicatedToken = nullptr;
    if (DuplicateTokenEx(hToken, MAXIMUM_ALLOWED, nullptr, SecurityIdentification, TokenPrimary, &hDuplicatedToken) == FALSE)
        return false;
    LPVOID lpEnvironment = nullptr;
    if (CreateEnvironmentBlock(&lpEnvironment, hDuplicatedToken, FALSE) == FALSE)
        return false;
    wchar_t *wcstring;
    if (!params.isEmpty())
    {
        QByteArray byteArray = params.toLocal8Bit();
        char *param = byteArray.data();
        size_t newsize = strlen(param) + 1;
        wcstring = new wchar_t[newsize];
        size_t convertedChars = 0;
        mbstowcs_s(&convertedChars, wcstring, newsize, param, _TRUNCATE);
    }
    if (CreateProcessAsUser(hDuplicatedToken,
                            reinterpret_cast<const wchar_t *>(QDir::toNativeSeparators(path).utf16()),
                            params.isEmpty() ? nullptr : wcstring, nullptr, nullptr, FALSE,
                            NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT,
                            lpEnvironment,
                            reinterpret_cast<const wchar_t *>(dir.utf16()),
                            &si, &pi) == FALSE)
        return false;
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

int Exit(int resultCode, bool trulyExit, HANDLE mutex, HWND workerw)
{
    if (mutex != nullptr)
    {
        ReleaseMutex(mutex);
        CloseHandle(mutex);
    }
    if (workerw != nullptr)
        ShowWindow(workerw, SW_HIDE);
    if (trulyExit)
        exit(resultCode);
    return resultCode;
}

bool run(const QString &path, const QStringList &params, bool needAdmin)
{
    if (path.isEmpty())
        return false;
    if (!QFileInfo::exists(path))
        return false;
    QString paramsInAll;
    if (!params.isEmpty())
        paramsInAll = params.join(QLatin1Char(' '));
    if (needAdmin)
        return adminRun(path, paramsInAll);
    DWORD processId = GetCurrentProcessId();
    DWORD sessionId;
    if (!ProcessIdToSessionId(processId, &sessionId))
        return false;
    if ((sessionId == static_cast<DWORD>(0)) || (sessionId != static_cast<DWORD>(1)))
        return launchSession1Process(path, paramsInAll);
    return QProcess::startDetached(QDir::toNativeSeparators(path), params, QDir::toNativeSeparators(QFileInfo(path).canonicalPath()));
}

bool killProcess(const QString &name)
{
    if (name.isEmpty())
        return false;
    PROCESSENTRY32 pe;
    DWORD id = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    pe.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(hSnapshot, &pe))
        return false;
    while (true)
    {
        pe.dwSize = sizeof(PROCESSENTRY32);
        if (Process32Next(hSnapshot, &pe) == FALSE)
            break;
        if (_tcscmp(pe.szExeFile, reinterpret_cast<const wchar_t *>(name.utf16())) == 0)
        {
            id = pe.th32ProcessID;
            break;
        }
    }
    CloseHandle(hSnapshot);
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, id);
    if (hProcess != nullptr)
    {
        TerminateProcess(hProcess, 0);
        CloseHandle(hProcess);
    }
    return true;
}

}