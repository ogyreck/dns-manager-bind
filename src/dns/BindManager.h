#pragma once
#include <QObject>
#include <QString>
#include <QList>
#include "model/Zone.h"
#include "model/ResourceRecord.h"

class BindManager : public QObject {
    Q_OBJECT
public:
    explicit BindManager(QObject *parent = nullptr);

    QList<Zone>           loadZones(const QString &namedConfPath, QString *error = nullptr);
    QList<ResourceRecord> loadZoneRecords(const QString &filePath, QString *error = nullptr);

    // Синхронные методы (используются в тестах и внутри saveZone/deleteZone)
    bool    start(QString *error);
    bool    stop(QString *error);
    bool    restart(QString *error);
    bool    reload(QString *error);

    // Асинхронные методы (для UI — не блокируют главный поток)
    void startAsync();
    void stopAsync();
    void restartAsync();
    void reloadAsync();

    bool    isRunning() const;
    QString version() const;

    // CRUD зон: запись файла + обновление named.conf + reload
    bool saveZone(const Zone &zone, const QString &namedConfPath, QString *error = nullptr);
    bool deleteZone(const QString &zoneName, const QString &filePath,
                    const QString &namedConfPath, QString *error = nullptr);

signals:
    void commandFinished(const QString &action, bool success, const QString &error);

private:
    void runSystemctlAsync(const QString &action, const QString &service);
};
