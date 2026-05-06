#pragma once
#include <QString>
#include <QList>
#include "model/Zone.h"
#include "model/ResourceRecord.h"

class BindManager {
public:
    QList<Zone>           loadZones(const QString &namedConfPath, QString *error = nullptr);
    QList<ResourceRecord> loadZoneRecords(const QString &filePath, QString *error = nullptr);

    bool    start(QString *error);
    bool    stop(QString *error);
    bool    restart(QString *error);
    bool    reload(QString *error);
    bool    isRunning() const;
    QString version() const;

    // CRUD зон: запись файла + обновление named.conf + reload
    bool saveZone(const Zone &zone, const QString &namedConfPath, QString *error = nullptr);
    bool deleteZone(const QString &zoneName, const QString &filePath,
                    const QString &namedConfPath, QString *error = nullptr);
};
