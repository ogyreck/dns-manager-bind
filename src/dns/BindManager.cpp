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

BindManager::BindManager(QObject *parent) : QObject(parent) {}

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

// Определяет имя сервиса BIND9 (bind9 или named) один раз и кэширует результат.
static QString detectServiceName() {
    static QString s_cached;
    if (!s_cached.isEmpty()) return s_cached;
    for (const QString svc : {"bind9", "named"}) {
        QProcess proc;
        proc.start("systemctl", {"cat", svc + ".service"});
        proc.waitForFinished(5000);
        if (proc.exitCode() == 0) {
            qDebug() << "[FIX] detectServiceName: using" << svc;
            s_cached = svc;
            return s_cached;
        }
    }
    qWarning() << "[FIX] detectServiceName: bind9 и named не найдены, использую named";
    s_cached = "named";
    return s_cached;
}

// Запускает systemctl <action> для BIND9-сервиса.
static bool runSystemctl(const QString &action, QString *error) {
    const QString svc = detectServiceName();
    const QStringList args = {action, svc};
    qDebug() << "[BindManager] systemctl" << args;
    QProcess proc;
    proc.start("systemctl", args);
    proc.waitForFinished(10000);
    const int code = proc.exitCode();
    qDebug() << "[BindManager] systemctl" << action << svc << "exitCode:" << code;
    if (code == 0) return true;
    const QString lastErr = proc.readAllStandardError().trimmed();
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

// Запускает systemctl <action> <service> асинхронно; эмитирует commandFinished.
// При ошибке старта дополнительно запускает named-checkconf для диагностики.
void BindManager::runSystemctlAsync(const QString &action, const QString &service) {
    auto *proc = new QProcess(this);
    qDebug() << "[BindManager]" << action << "Async: launched systemctl" << action << service;
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc, action](int code, QProcess::ExitStatus) {
                proc->deleteLater();
                if (code == 0) {
                    qDebug() << "[BindManager] commandFinished: action=" << action << "success=true";
                    emit commandFinished(action, true, {});
                    return;
                }
                QString err = proc->readAllStandardError().trimmed();
                if (action == "start") {
                    QProcess checkConf;
                    checkConf.start("named-checkconf", QStringList{});
                    if (checkConf.waitForStarted(3000) && checkConf.waitForFinished(5000)) {
                        const QString confOut = (checkConf.readAllStandardOutput() +
                                                 checkConf.readAllStandardError()).trimmed();
                        if (!confOut.isEmpty()) {
                            qWarning() << "[FIX] startAsync failed, checkconf output:" << confOut;
                            err += "\n\nnamed-checkconf:\n" + confOut;
                        }
                    }
                }
                qWarning() << "[BindManager] commandFinished: action=" << action
                           << "success=false err=" << err;
                emit commandFinished(action, false, err);
            });
    proc->start("systemctl", {action, service});
}

void BindManager::startAsync() {
    qDebug() << "[BindManager] startAsync: launched";
    runSystemctlAsync("start", detectServiceName());
}

void BindManager::stopAsync() {
    qDebug() << "[BindManager] stopAsync: launched";
    runSystemctlAsync("stop", detectServiceName());
}

void BindManager::restartAsync() {
    qDebug() << "[BindManager] restartAsync: launched";
    runSystemctlAsync("restart", detectServiceName());
}

void BindManager::reloadAsync() {
    qDebug() << "[BindManager] reloadAsync: launched";
    runSystemctlAsync("reload", detectServiceName());
}

bool BindManager::isRunning() const {
    qDebug() << "[BindManager] isRunning";
    const QString svc = detectServiceName();
    QProcess proc;
    proc.start("systemctl", {"is-active", svc});
    proc.waitForFinished(5000);
    const bool running = (proc.exitCode() == 0);
    qDebug() << "[BindManager] isRunning:" << running << "(service=" << svc << ")";
    return running;
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

// Добавляет или обновляет блок zone "name" { ... }; в named.conf.
// После записи валидирует через named-checkconf; при ошибке откатывает файл.
static bool updateNamedConf(const Zone &zone, const QString &namedConfPath, QString *error) {
    qDebug() << "[BindManager] updateNamedConf zone=" << zone.name << "conf=" << namedConfPath;
    QFile file(namedConfPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString err = "Не удалось открыть named.conf: " + namedConfPath;
        qWarning() << "[BindManager]" << err;
        if (error) *error = err;
        return false;
    }
    const QString originalContent = file.readAll();
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
    QString content = originalContent;
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
    QTextStream streamOut(&file);
    streamOut << content;
    file.close();

    // Валидируем конфиг; при ошибке откатываем файл
    QProcess checkProc;
    checkProc.start("named-checkconf", {namedConfPath});
    if (!checkProc.waitForStarted(3000) || !checkProc.waitForFinished(10000)) {
        qWarning() << "[BindManager] updateNamedConf: named-checkconf не запустился, пропускаем валидацию";
    } else if (checkProc.exitCode() != 0) {
        const QString checkErr = (checkProc.readAllStandardOutput() +
                                   checkProc.readAllStandardError()).trimmed();
        qWarning() << "[BindManager] updateNamedConf: checkconf failed:" << checkErr;
        if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            QTextStream restore(&file);
            restore << originalContent;
            file.close();
            qWarning() << "[BindManager] updateNamedConf: named.conf восстановлен";
        } else {
            qWarning() << "[BindManager] updateNamedConf: не удалось восстановить named.conf!";
        }
        if (error) *error = "named-checkconf: " + checkErr;
        return false;
    } else {
        qDebug() << "[BindManager] updateNamedConf: checkconf passed";
    }

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

    Zone zoneToWrite = zone;

    // ZoneWizard гарантирует наличие SOA и NS — здесь только диагностика
    bool hasSoa = false, hasNs = false;
    for (const ResourceRecord &rr : zone.records) {
        if (rr.type == RecordType::SOA) hasSoa = true;
        if (rr.type == RecordType::NS)  hasNs  = true;
    }
    if (!hasSoa || !hasNs) {
        qWarning() << "[BindManager] saveZone: зона" << zone.name
                   << "не содержит SOA или NS — валидация скорее всего завершится ошибкой";
    }

    ConfigValidator validator;
    ConfigWriter writer;

    // Записываем файл зоны атомарно
    if (!writer.writeZoneFile(zoneToWrite, zone.filePath, &validator, error)) {
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
