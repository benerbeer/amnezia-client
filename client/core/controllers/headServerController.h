#ifndef HEADSERVERCONTROLLER_H
#define HEADSERVERCONTROLLER_H

#include <QJsonObject>
#include <QObject>
#include <QPointer>

#include "core/controllers/selfhosted/importController.h"
#include "core/repositories/secureAppSettingsRepository.h"
#include "core/repositories/secureServersRepository.h"

class QNetworkReply;

class HeadServerController : public QObject
{
    Q_OBJECT

public:
    explicit HeadServerController(SecureServersRepository *serversRepository, SecureAppSettingsRepository *appSettingsRepository,
                                  ImportController *importController, QObject *parent = nullptr);

    bool isConfigured() const;
    bool isBusy() const;
    QUrl endpoint(const QString &path) const;
    void sync();

signals:
    void syncFinished(bool updated, const QString &message);

private:
    QString findServerIdByHostName(const QString &hostName) const;
    void selectServer(const QString &serverId);
    void requestConfig(const QString &ip, const QString &revision, const QString &existingServerId);
    void finishWithConfig(const QString &ip, const QString &revision, const QString &config, const QString &existingServerId);
    void finish(bool updated, const QString &message);

    SecureServersRepository *m_serversRepository;
    SecureAppSettingsRepository *m_appSettingsRepository;
    ImportController *m_importController;
    QPointer<QNetworkReply> m_reply;
};

#endif // HEADSERVERCONTROLLER_H
