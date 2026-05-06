#include "dns/BindManager.h"
#include "parser/NamedConfParser.h"
#include "parser/ZoneFileParser.h"
#include "parser/ConfigWriter.h"
#include "dns/ConfigValidator.h"
#include <QDebug>
#include <QFile>
#include <QProcess>
#include <QRegExp>
#include <QTextStream>

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

// Запускает systemctl <action> named; при ошибке пробует bind9.
static bool runSystemctl(const QString &action, QString *error) {
    QString lastErr;
    for (const QString svc : {"named", "bind9"}) {
        const QStringList args = {action, svc};
        qDebug() << "[BindManager] systemctl" << args;
        QProcess proc;
        proc.start("systemctl", args);
        proc.waitForFinished(10000);
        int code = proc.exitCode();
        qDebug() << "[BindManager] systemctl" << action << svc << "exitCode:" << code;
        if (code == 0) return true;
        lastErr = proc.readAllStandardError().trimmed();
    }
    qWarning() << "[BindManager]" << action << "failed:" << lastErr;
    if (error) *error = lastErr;
    return false;
}

bool BindManager::start(QString *error) {
    qDebug() << "[BindManager] start";
    return runSystemctl("start", error);
}

bool BindManager::stop(QString *error) {
    qDebug() << "[BindManager] stop";
    return runSystemctl("stop", error);
}

bool BindManager::restart(QString *error) {
    qDebug() << "[BindManager] restart";
    return runSystemctl("restart", error);
}

bool BindManager::reload(QString *error) {
    qDebug() << "[BindManager] reload";
    return runSystemctl("reload", error);
}

bool BindManager::isRunning() const {
    qDebug() << "[BindManager] isRunning";
    for (const QString svc : {"named", "bind9"}) {
        QProcess proc;
        proc.start("systemctl", {"is-active", svc});
        proc.waitForFinished(5000);
        if (proc.exitCode() == 0) {
            qDebug() << "[BindManager] isRunning: true (service=" << svc << ")";
            return true;
        }
    }
    qDebug() << "[BindManager] isRunning: false";
    return false;
}

QString BindManager::version() const {
    qDebug() << "[BindManager] version";
    QProcess proc;
    proc.start("named", {"-v"});
    proc.waitForFinished(5000);
    QString out = proc.readAllStandardOutput().trimmed();
    if (out.isEmpty()) out = proc.readAllStandardError().trimmed();
    const QString ver = out.split('\n').first().trimmed();
    qDebug() << "[BindManager] version:" << ver;
    return ver;
}

// Добавляет или обновляет блок zone "name" { ... }; в named.conf
static bool updateNamedConf(const Zone &zone, const QString &namedConfPath, QString *error) {
    qDebug() << "[BindManager] updateNamedConf zone=" << zone.name << "conf=" << namedConfPath;
    QFile file(namedConfPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString err = "Не удалось открыть named.conf: " + namedConfPath;
        qWarning() << "[BindManager]" << err;
        if (error) *error = err;
        return false;
    }
    QString content = file.readAll();
    file.close();

    // Формируем блок зоны
    QString typeStr = (zone.type == ZoneType::Master) ? "master" : "slave";
    QString newBlock = QString("\nzone \"%1\" {\n    type %2;\n    file \"%3\";\n};\n")
                           .arg(zone.name, typeStr, zone.filePath);
    if (zone.type == ZoneType::Slave && !zone.masterIp.isEmpty()) {
        newBlock = QString("\nzone \"%1\" {\n    type slave;\n    file \"%2\";\n    masters { %3; };\n};\n")
                       .arg(zone.name, zone.filePath, zone.masterIp);
    }

    // Если зона уже есть — заменяем блок, иначе дописываем
    QRegExp rx(QString("zone\\s+\"%1\"\\s*\\{[^}]*\\};").arg(QRegExp::escape(zone.name)));
    rx.setMinimal(true);
    if (content.contains(rx)) {
        content.replace(rx, newBlock.trimmed());
        qDebug() << "[BindManager] updateNamedConf: replaced existing block";
    } else {
        content += newBlock;
        qDebug() << "[BindManager] updateNamedConf: appended new block";
    }

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QString err = "Не удалось записать named.conf: " + namedConfPath;
        qWarning() << "[BindManager]" << err;
        if (error) *error = err;
        return false;
    }
    QTextStream out(&file);
    out << content;
    file.close();
    qDebug() << "[BindManager] updateNamedConf: done";
    return true;
}

// Удаляет блок zone "name" { ... }; из named.conf
static bool removeZoneFromConf(const QString &zoneName, const QString &namedConfPath, QString *error) {
    qDebug() << "[BindManager] removeZoneFromConf zone=" << zoneName;
    QFile file(namedConfPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString err = "Не удалось открыть named.conf: " + namedConfPath;
        qWarning() << "[BindManager]" << err;
        if (error) *error = err;
        return false;
    }
    QString content = file.readAll();
    file.close();

    QRegExp rx(QString("\\n?zone\\s+\"%1\"\\s*\\{[^}]*\\};").arg(QRegExp::escape(zoneName)));
    rx.setMinimal(true);
    content.remove(rx);
    qDebug() << "[BindManager] removeZoneFromConf: block removed";

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QString err = "Не удалось записать named.conf: " + namedConfPath;
        qWarning() << "[BindManager]" << err;
        if (error) *error = err;
        return false;
    }
    QTextStream out(&file);
    out << content;
    file.close();
    return true;
}

bool BindManager::saveZone(const Zone &zone, const QString &namedConfPath, QString *error) {
    qDebug() << "[BindManager] saveZone zone=" << zone.name;

    ConfigValidator validator;
    ConfigWriter writer;

    // Записываем файл зоны атомарно
    if (!writer.writeZoneFile(zone, zone.filePath, &validator, error)) {
        qWarning() << "[BindManager] saveZone: writeZoneFile failed";
        return false;
    }
    qDebug() << "[BindManager] saveZone: zone file written";

    // Обновляем named.conf
    if (!updateNamedConf(zone, namedConfPath, error)) {
        qWarning() << "[BindManager] saveZone: updateNamedConf failed";
        return false;
    }

    // Перезагружаем конфигурацию
    if (!reload(error)) {
        qWarning() << "[BindManager] saveZone: reload failed";
        return false;
    }

    qDebug() << "[BindManager] saveZone: done";
    return true;
}

bool BindManager::deleteZone(const QString &zoneName, const QString &filePath,
                              const QString &namedConfPath, QString *error) {
    qDebug() << "[BindManager] deleteZone zone=" << zoneName;

    // Удаляем файл зоны
    if (QFile::exists(filePath)) {
        if (!QFile::remove(filePath)) {
            QString err = "Не удалось удалить файл зоны: " + filePath;
            qWarning() << "[BindManager]" << err;
            if (error) *error = err;
            return false;
        }
        qDebug() << "[BindManager] deleteZone: zone file removed";
    } else {
        qWarning() << "[BindManager] deleteZone: zone file not found:" << filePath;
    }

    // Убираем блок из named.conf
    if (!removeZoneFromConf(zoneName, namedConfPath, error)) {
        qWarning() << "[BindManager] deleteZone: removeZoneFromConf failed";
        return false;
    }

    // Перезагружаем конфигурацию
    if (!reload(error)) {
        qWarning() << "[BindManager] deleteZone: reload failed";
        return false;
    }

    qDebug() << "[BindManager] deleteZone: done";
    return true;
}
