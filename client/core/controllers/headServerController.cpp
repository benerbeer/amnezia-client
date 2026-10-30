#include "headServerController.h"

#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

#include "amneziaApplication.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/serverConfigUtils.h"

namespace
{
    constexpr int requestTimeoutMs = 10000;
    constexpr int httpForbidden = 403;
    constexpr int httpConflict = 409;
}

HeadServerController::HeadServerController(SecureServersRepository *serversRepository, SecureAppSettingsRepository *appSettingsRepository,
                                           ImportController *importController, QObject *parent)
    : QObject(parent),
      m_serversRepository(serversRepository),
      m_appSettingsRepository(appSettingsRepository),
      m_importController(importController)
{
}

bool HeadServerController::isConfigured() const
{
    return !m_appSettingsRepository->headServerUrl().trimmed().isEmpty();
}

bool HeadServerController::isBusy() const
{
    return !m_reply.isNull();
}

QUrl HeadServerController::endpoint(const QString &path) const
{
    QString base = m_appSettingsRepository->headServerUrl().trimmed();
    while (base.endsWith('/')) {
        base.chop(1);
    }
    if (!base.contains("://")) {
        base.prepend("https://");
    }
    return QUrl(base + path);
}

QString HeadServerController::findServerIdByHostName(const QString &hostName) const
{
    const QVector<QString> ids = m_serversRepository->orderedServerIds();
    for (const QString &id : ids) {
        const auto json = m_serversRepository->serverJsonById(id);
        if (json.has_value() && json->value(configKey::hostName).toString() == hostName) {
            return id;
        }
    }
    return QString();
}

void HeadServerController::sync()
{
    if (isBusy()) {
        return;
    }

    QNetworkRequest request(endpoint("/api/v1/active"));
    request.setTransferTimeout(requestTimeoutMs);

    m_reply = amnApp->networkManager()->get(request);
    connect(m_reply, &QNetworkReply::finished, this, [this]() {
        QNetworkReply *reply = m_reply;
        reply->deleteLater();
        m_reply.clear();

        if (reply->error() != QNetworkReply::NoError) {
            finish(false, tr("Head server is unavailable: %1").arg(reply->errorString()));
            return;
        }

        const QJsonObject active = QJsonDocument::fromJson(reply->readAll()).object();
        const QString ip = active.value("ip").toString();
        const QString revision = active.value("rev").toString();
        if (ip.isEmpty()) {
            finish(false, tr("Head server returned an invalid response"));
            return;
        }

        const QString serverId = findServerIdByHostName(ip);
        if (serverId.isEmpty()) {
            requestConfig(ip, revision, QString());
            return;
        }

        const bool isOutdated = !revision.isEmpty() && m_appSettingsRepository->headServerRevision(ip) != revision
                && m_serversRepository->serverKind(serverId) == serverConfigUtils::ConfigType::SelfHostedUser;
        if (isOutdated) {
            requestConfig(ip, revision, serverId);
            return;
        }

        selectServer(serverId);
    });
}

void HeadServerController::selectServer(const QString &serverId)
{
    const bool changed = m_serversRepository->defaultServerId() != serverId;
    if (changed) {
        m_serversRepository->setDefaultServer(serverId);
    }
    finish(changed, QString());
}

void HeadServerController::requestConfig(const QString &ip, const QString &revision, const QString &existingServerId)
{
    const QString phone = m_appSettingsRepository->headServerPhone().trimmed();
    const QString pin = m_appSettingsRepository->headServerPin().trimmed();
    if (phone.isEmpty() || pin.isEmpty()) {
        if (existingServerId.isEmpty()) {
            finish(false, tr("A new VPN server is available, but the phone number and PIN are not set"));
        } else {
            selectServer(existingServerId);
        }
        return;
    }

    QNetworkRequest request(endpoint("/api/v1/config"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(requestTimeoutMs * 6);

    const QJsonObject body {
        { "phone", phone },
        { "pin", pin },
        { "device_id", m_appSettingsRepository->headServerDeviceId() },
        { "ip", ip },
    };

    m_reply = amnApp->networkManager()->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(m_reply, &QNetworkReply::finished, this, [this, ip, revision, existingServerId]() {
        QNetworkReply *reply = m_reply;
        reply->deleteLater();
        m_reply.clear();

        const auto failWith = [this, &existingServerId](const QString &message) {
            if (!existingServerId.isEmpty()) {
                m_serversRepository->setDefaultServer(existingServerId);
            }
            finish(false, message);
        };

        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status == httpForbidden) {
            failWith(tr("Access denied: check the phone number and PIN"));
            return;
        }
        if (status == httpConflict) {
            sync();
            return;
        }
        if (reply->error() != QNetworkReply::NoError) {
            failWith(tr("Failed to get settings for the new server: %1").arg(reply->errorString()));
            return;
        }

        const QJsonObject response = QJsonDocument::fromJson(reply->readAll()).object();
        const QString receivedRevision = response.value("rev").toString(revision);
        finishWithConfig(response.value("ip").toString(ip), receivedRevision, response.value("config").toString(), existingServerId);
    });
}

void HeadServerController::finishWithConfig(const QString &ip, const QString &revision, const QString &config, const QString &existingServerId)
{
    const auto result = m_importController->extractConfigFromData(config);
    if (result.errorCode != ErrorCode::NoError || result.config.isEmpty()) {
        finish(false, tr("The server settings received from the head server are invalid"));
        return;
    }

    const serverConfigUtils::ConfigType kind = serverConfigUtils::configTypeFromJson(result.config);
    QString serverId = existingServerId;
    if (serverId.isEmpty()) {
        serverId = m_serversRepository->addServer(QString(), result.config, kind);
    } else {
        m_serversRepository->editServer(serverId, result.config, kind);
    }

    if (!m_serversRepository->serverJsonById(serverId).has_value()) {
        finish(false, tr("Failed to save the new server settings"));
        return;
    }

    m_serversRepository->setDefaultServer(serverId);
    m_appSettingsRepository->setHeadServerRevision(ip, revision);
    finish(true, existingServerId.isEmpty() ? tr("Settings for the new server %1 were applied").arg(ip)
                                            : tr("Settings for the server %1 were updated").arg(ip));
}

void HeadServerController::finish(bool updated, const QString &message)
{
    emit syncFinished(updated, message);
}
