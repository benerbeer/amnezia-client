#ifndef HEADSERVERUICONTROLLER_H
#define HEADSERVERUICONTROLLER_H

#include <QObject>

#include "core/controllers/headServerController.h"
#include "core/controllers/headServerUpdateController.h"
#include "core/repositories/secureAppSettingsRepository.h"

class HeadServerUiController : public QObject
{
    Q_OBJECT
public:
    explicit HeadServerUiController(HeadServerController *headServerController, HeadServerUpdateController *updateController,
                                    SecureAppSettingsRepository *appSettingsRepository, QObject *parent = nullptr);

    Q_PROPERTY(QString url READ url WRITE setUrl NOTIFY settingsChanged)
    Q_PROPERTY(QString phone READ phone WRITE setPhone NOTIFY settingsChanged)
    Q_PROPERTY(QString pin READ pin WRITE setPin NOTIFY settingsChanged)
    Q_PROPERTY(bool isBusy READ isBusy NOTIFY busyChanged)
    Q_PROPERTY(bool updateSupported READ updateSupported NOTIFY updateStateChanged)
    Q_PROPERTY(bool updateChecking READ updateChecking NOTIFY updateStateChanged)
    Q_PROPERTY(bool updateAvailable READ updateAvailable NOTIFY updateStateChanged)
    Q_PROPERTY(bool updateDownloading READ updateDownloading NOTIFY updateStateChanged)
    Q_PROPERTY(QString updateVersion READ updateVersion NOTIFY updateStateChanged)
    Q_PROPERTY(QString updateNotes READ updateNotes NOTIFY updateStateChanged)
    Q_PROPERTY(QString updateSizeText READ updateSizeText NOTIFY updateStateChanged)
    Q_PROPERTY(int updateProgress READ updateProgress NOTIFY updateProgressChanged)

public slots:
    QString url() const;
    void setUrl(const QString &url);
    QString phone() const;
    void setPhone(const QString &phone);
    QString pin() const;
    void setPin(const QString &pin);
    bool isBusy() const;

    void checkNow();

    bool updateSupported() const;
    bool updateChecking() const;
    bool updateAvailable() const;
    bool updateDownloading() const;
    QString updateVersion() const;
    QString updateNotes() const;
    QString updateSizeText() const;
    int updateProgress() const;
    void checkForUpdate();
    void installUpdate();

signals:
    void settingsChanged();
    void busyChanged();
    void checkFinished(bool updated, const QString &message);
    void updateStateChanged();
    void updateProgressChanged();
    void updateFound();
    void updateMessage(const QString &message);
    void updateCheckFinished(bool found, const QString &message);

private:
    HeadServerController *m_headServerController;
    HeadServerUpdateController *m_updateController;
    SecureAppSettingsRepository *m_appSettingsRepository;
};

#endif // HEADSERVERUICONTROLLER_H
