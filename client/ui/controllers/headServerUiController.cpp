#include "headServerUiController.h"

#include <QLocale>

HeadServerUiController::HeadServerUiController(HeadServerController *headServerController, HeadServerUpdateController *updateController,
                                               SecureAppSettingsRepository *appSettingsRepository, QObject *parent)
    : QObject(parent),
      m_headServerController(headServerController),
      m_updateController(updateController),
      m_appSettingsRepository(appSettingsRepository)
{
    connect(m_updateController, &HeadServerUpdateController::stateChanged, this, &HeadServerUiController::updateStateChanged);
    connect(m_updateController, &HeadServerUpdateController::progressChanged, this, &HeadServerUiController::updateProgressChanged);
    connect(m_updateController, &HeadServerUpdateController::updateFound, this, &HeadServerUiController::updateFound);
    connect(m_updateController, &HeadServerUpdateController::messageOccurred, this, &HeadServerUiController::updateMessage);
    connect(m_updateController, &HeadServerUpdateController::checkFinished, this, &HeadServerUiController::updateCheckFinished);

    connect(m_headServerController, &HeadServerController::syncFinished, this, [this](bool updated, const QString &message) {
        emit busyChanged();
        emit checkFinished(updated, message);
    });
}

QString HeadServerUiController::url() const
{
    return m_appSettingsRepository->headServerUrl();
}

void HeadServerUiController::setUrl(const QString &url)
{
    m_appSettingsRepository->setHeadServerUrl(url.trimmed());
    emit settingsChanged();
}

QString HeadServerUiController::phone() const
{
    return m_appSettingsRepository->headServerPhone();
}

void HeadServerUiController::setPhone(const QString &phone)
{
    m_appSettingsRepository->setHeadServerPhone(phone.trimmed());
    emit settingsChanged();
}

QString HeadServerUiController::pin() const
{
    return m_appSettingsRepository->headServerPin();
}

void HeadServerUiController::setPin(const QString &pin)
{
    m_appSettingsRepository->setHeadServerPin(pin.trimmed());
    emit settingsChanged();
}

bool HeadServerUiController::isBusy() const
{
    return m_headServerController->isBusy();
}

void HeadServerUiController::checkNow()
{
    if (!m_headServerController->isConfigured()) {
        emit checkFinished(false, tr("Head server address is not set"));
        return;
    }
    m_headServerController->sync();
    emit busyChanged();
}

bool HeadServerUiController::updateSupported() const
{
    return m_updateController->isSupported();
}

bool HeadServerUiController::updateChecking() const
{
    return m_updateController->state() == HeadServerUpdateController::State::Checking;
}

bool HeadServerUiController::updateAvailable() const
{
    const auto state = m_updateController->state();
    return state == HeadServerUpdateController::State::UpdateAvailable || state == HeadServerUpdateController::State::Downloading
            || state == HeadServerUpdateController::State::ReadyToInstall;
}

bool HeadServerUiController::updateDownloading() const
{
    return m_updateController->state() == HeadServerUpdateController::State::Downloading;
}

QString HeadServerUiController::updateVersion() const
{
    return m_updateController->versionName();
}

QString HeadServerUiController::updateNotes() const
{
    return m_updateController->notes();
}

QString HeadServerUiController::updateSizeText() const
{
    return QLocale().formattedDataSize(m_updateController->size());
}

int HeadServerUiController::updateProgress() const
{
    return m_updateController->progress();
}

void HeadServerUiController::checkForUpdate()
{
    if (!m_updateController->isSupported()) {
        emit updateCheckFinished(false, tr("Updates through the head server are not available in this build"));
        return;
    }
    m_updateController->check();
}

void HeadServerUiController::installUpdate()
{
    m_updateController->downloadAndInstall();
}
