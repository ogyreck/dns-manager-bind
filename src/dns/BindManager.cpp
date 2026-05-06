#include "dns/BindManager.h"
#include "parser/NamedConfParser.h"
#include "parser/ZoneFileParser.h"
#include <QDebug>

QList<Zone> BindManager::loadZones(const QString &namedConfPath, QString *error) {
    qDebug() << "[BindManager] loadZones:" << namedConfPath;
    NamedConfParser parser;
    QList<Zone> zones = parser.parse(namedConfPath);
    if (!parser.lastError().isEmpty()) {
        qWarning() << "[BindManager] loadZones error:" << parser.lastError();
        if (error) *error = parser.lastError();
    }
    return zones;
}

QList<ResourceRecord> BindManager::loadZoneRecords(const QString &filePath, QString *error) {
    qDebug() << "[BindManager] loadZoneRecords:" << filePath;
    ZoneFileParser parser;
    QList<ResourceRecord> records = parser.parse(filePath);
    if (!parser.lastError().isEmpty()) {
        qWarning() << "[BindManager] loadZoneRecords error:" << parser.lastError();
        if (error) *error = parser.lastError();
    }
    return records;
}

bool BindManager::start(QString *error) {
    Q_UNUSED(error)
    qDebug() << "[BindManager] stub:" << Q_FUNC_INFO;
    return false;
}

bool BindManager::stop(QString *error) {
    Q_UNUSED(error)
    qDebug() << "[BindManager] stub:" << Q_FUNC_INFO;
    return false;
}

bool BindManager::restart(QString *error) {
    Q_UNUSED(error)
    qDebug() << "[BindManager] stub:" << Q_FUNC_INFO;
    return false;
}

bool BindManager::reload(QString *error) {
    Q_UNUSED(error)
    qDebug() << "[BindManager] stub:" << Q_FUNC_INFO;
    return false;
}

bool BindManager::isRunning() const {
    qDebug() << "[BindManager] stub:" << Q_FUNC_INFO;
    return false;
}

QString BindManager::version() const {
    qDebug() << "[BindManager] stub:" << Q_FUNC_INFO;
    return {};
}
