#ifndef HEADSERVERUPDATECONTROLLER_H
#define HEADSERVERUPDATECONTROLLER_H

#include <QCryptographicHash>
#include <QFile>
#include <QObject>
#include <QPointer>

#include "core/controllers/headServerController.h"

class QNetworkReply;

class HeadServerUpdateController : public QObject
{
    Q_OBJECT

public:
    enum class State {
        Idle,
        Checking,
        UpdateAvailable,
        Downloading,
        ReadyToInstall,
    };

    explicit HeadServerUpdateController(HeadServerController *headServerController, QObject *parent = nullptr);

    bool isSupported() const;
    State state() const;
    QString versionName() const;
    QString notes() const;
    qint64 size() const;
    int progress() const;

    void check();
    void downloadAndInstall();

signals:
    void stateChanged();
    void progressChanged();
    void updateFound();
    void checkFinished(bool found, const QString &message);
    void messageOccurred(const QString &message);

private:
    static QString updatesDir();
    static QString abi();
    QString apkPath() const;
    void setState(State state);
    void removeStaleFiles(qint64 currentVersionCode) const;
    void startDownload();
    void finishDownload(QNetworkReply *reply);
    void install();
    void fail(const QString &message);

    HeadServerController *m_headServerController;
    State m_state = State::Idle;
    qint64 m_versionCode = 0;
    QString m_versionName;
    QString m_notes;
    qint64 m_size = 0;
    QString m_sha256;
    QString m_downloadPath;
    int m_progress = 0;
    QFile m_file;
    QCryptographicHash m_hash { QCryptographicHash::Sha256 };
    qint64 m_received = 0;
    QPointer<QNetworkReply> m_reply;
};

#endif // HEADSERVERUPDATECONTROLLER_H
