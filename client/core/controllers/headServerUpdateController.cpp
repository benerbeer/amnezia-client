#include "headServerUpdateController.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QSysInfo>
#include <QUrlQuery>

#include "amneziaApplication.h"

#if defined(Q_OS_ANDROID)
    #include "platforms/android/android_controller.h"
#endif

namespace
{
    constexpr int requestTimeoutMs = 15000;
    constexpr int downloadIdleTimeoutMs = 30000;
    constexpr int installStarted = 0;
    constexpr int installPermissionRequired = 1;
    const QString downloadPathPrefix = QStringLiteral("/api/v1/app/download/");
}

HeadServerUpdateController::HeadServerUpdateController(HeadServerController *headServerController, QObject *parent)
    : QObject(parent), m_headServerController(headServerController)
{
}

bool HeadServerUpdateController::isSupported() const
{
#if defined(Q_OS_ANDROID)
    return !AndroidController::instance()->isPlay() && m_headServerController->isConfigured();
#else
    return false;
#endif
}

HeadServerUpdateController::State HeadServerUpdateController::state() const
{
    return m_state;
}

QString HeadServerUpdateController::versionName() const
{
    return m_versionName;
}

QString HeadServerUpdateController::notes() const
{
    return m_notes;
}

qint64 HeadServerUpdateController::size() const
{
    return m_size;
}

int HeadServerUpdateController::progress() const
{
    return m_progress;
}

QString HeadServerUpdateController::updatesDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/updates";
}

QString HeadServerUpdateController::abi()
{
    const QString arch = QSysInfo::currentCpuArchitecture();
    if (arch == "arm64") {
        return "arm64-v8a";
    }
    if (arch == "arm") {
        return "armeabi-v7a";
    }
    if (arch == "i386") {
        return "x86";
    }
    return arch;
}

QString HeadServerUpdateController::apkPath() const
{
    return QString("%1/AmneziaVPN-%2.apk").arg(updatesDir()).arg(m_versionCode);
}

void HeadServerUpdateController::setState(State state)
{
    if (m_state == state) {
        return;
    }
    m_state = state;
    emit stateChanged();
}

void HeadServerUpdateController::removeStaleFiles(qint64 currentVersionCode) const
{
    const QDir dir(updatesDir());
    const QStringList files = dir.entryList({ "*.apk", "*.part" }, QDir::Files);
    for (const QString &name : files) {
        const qint64 code = name.section('-', 1).section('.', 0, 0).toLongLong();
        if (name.endsWith(".part") || code <= currentVersionCode) {
            QFile::remove(dir.filePath(name));
        }
    }
}

void HeadServerUpdateController::fail(const QString &message)
{
    m_file.close();
    QFile::remove(m_file.fileName());
    setState(m_versionCode > 0 ? State::UpdateAvailable : State::Idle);
    emit messageOccurred(message);
}

void HeadServerUpdateController::check()
{
#if defined(Q_OS_ANDROID)
    if (!isSupported() || m_state == State::Checking || m_state == State::Downloading) {
        return;
    }

    const qint64 currentVersionCode = AndroidController::instance()->versionCode();
    removeStaleFiles(currentVersionCode);
    setState(State::Checking);

    QUrl url = m_headServerController->endpoint("/api/v1/app/update");
    QUrlQuery query;
    query.addQueryItem("version_code", QString::number(currentVersionCode));
    query.addQueryItem("abi", abi());
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setTransferTimeout(requestTimeoutMs);

    QNetworkReply *reply = amnApp->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            setState(State::Idle);
            emit checkFinished(false, tr("Failed to check for updates: %1").arg(reply->errorString()));
            return;
        }

        const QJsonObject data = QJsonDocument::fromJson(reply->readAll()).object();
        const QString path = data.value("url").toString();
        if (!data.value("update").toBool() || !path.startsWith(downloadPathPrefix)) {
            setState(State::Idle);
            emit checkFinished(false, QString());
            return;
        }

        m_versionCode = data.value("version_code").toInteger();
        m_versionName = data.value("version_name").toString();
        m_notes = data.value("notes").toString();
        m_size = data.value("size").toInteger();
        m_sha256 = data.value("sha256").toString().toLower();
        m_downloadPath = path;
        m_progress = 0;
        emit progressChanged();

        setState(QFile::exists(apkPath()) && QFileInfo(apkPath()).size() == m_size ? State::ReadyToInstall : State::UpdateAvailable);
        emit updateFound();
        emit checkFinished(true, QString());
    });
#else
    emit checkFinished(false, QString());
#endif
}

void HeadServerUpdateController::downloadAndInstall()
{
    if (m_state == State::ReadyToInstall) {
        install();
    } else if (m_state == State::UpdateAvailable) {
        startDownload();
    }
}

void HeadServerUpdateController::startDownload()
{
    QDir().mkpath(updatesDir());
    m_file.setFileName(apkPath() + ".part");
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit messageOccurred(tr("Failed to save the update file: %1").arg(m_file.errorString()));
        return;
    }

    m_hash.reset();
    m_received = 0;
    m_progress = 0;
    emit progressChanged();
    setState(State::Downloading);

    QNetworkRequest request(m_headServerController->endpoint(m_downloadPath));
    request.setTransferTimeout(downloadIdleTimeoutMs);

    m_reply = amnApp->networkManager()->get(request);
    connect(m_reply, &QNetworkReply::readyRead, this, [this]() {
        const QByteArray chunk = m_reply->readAll();
        if (m_file.write(chunk) != chunk.size()) {
            m_reply->abort();
            return;
        }
        m_hash.addData(chunk);
        m_received += chunk.size();
        const int progress = m_size > 0 ? static_cast<int>(m_received * 100 / m_size) : 0;
        if (progress != m_progress) {
            m_progress = progress;
            emit progressChanged();
        }
    });
    connect(m_reply, &QNetworkReply::finished, this, [this]() {
        QNetworkReply *reply = m_reply;
        reply->deleteLater();
        m_reply.clear();
        finishDownload(reply);
    });
}

void HeadServerUpdateController::finishDownload(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        fail(tr("Failed to download the update: %1").arg(reply->errorString()));
        return;
    }

    const QByteArray tail = reply->readAll();
    if (!tail.isEmpty()) {
        m_file.write(tail);
        m_hash.addData(tail);
        m_received += tail.size();
    }
    m_file.close();

    if (m_received != m_size || QString::fromLatin1(m_hash.result().toHex()) != m_sha256) {
        fail(tr("The downloaded update is corrupted, try again"));
        return;
    }

    QFile::remove(apkPath());
    if (!QFile::rename(m_file.fileName(), apkPath())) {
        fail(tr("Failed to save the update file"));
        return;
    }

    m_progress = 100;
    emit progressChanged();
    setState(State::ReadyToInstall);
    install();
}

void HeadServerUpdateController::install()
{
#if defined(Q_OS_ANDROID)
    const int result = AndroidController::instance()->installApk(apkPath());
    if (result == installPermissionRequired) {
        emit messageOccurred(tr("Allow installing apps from Amnezia in the system settings, then press Update again"));
    } else if (result != installStarted) {
        emit messageOccurred(tr("Failed to start the installer"));
    }
#endif
}
