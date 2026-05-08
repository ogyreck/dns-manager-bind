#include "dns/ConfigValidator.h"
#include <QDebug>
#include <QProcess>

bool ConfigValidator::checkConf(const QString &namedConfPath, QString *error) {
    const QString program = "named-checkconf";
    const QStringList args = {namedConfPath};

    qDebug() << "[ConfigValidator] checkConf:" << program << args;

    QProcess proc;
    proc.start(program, args);
    proc.waitForFinished(10000);

    int exitCode = proc.exitCode();
    qDebug() << "[ConfigValidator] checkConf exitCode:" << exitCode;

    if (exitCode != 0) {
        // named-checkconf пишет ошибки в stdout
        QString err = proc.readAllStandardOutput().trimmed();
        if (err.isEmpty()) err = proc.readAllStandardError().trimmed();
        qWarning() << "[ConfigValidator] checkConf failed:" << err;
        if (error) *error = err;
        return false;
    }
    return true;
}

bool ConfigValidator::checkZone(const QString &zoneName, const QString &zoneFilePath, QString *error) {
    const QString program = "named-checkzone";
    const QStringList args = {zoneName, zoneFilePath};

    qDebug() << "[ConfigValidator] checkZone:" << program << args;

    QProcess proc;
    proc.start(program, args);
    proc.waitForFinished(10000);

    int exitCode = proc.exitCode();
    qDebug() << "[ConfigValidator] checkZone exitCode:" << exitCode;

    if (exitCode != 0) {
        // named-checkzone тоже может писать в stdout
        QString err = proc.readAllStandardOutput().trimmed();
        if (err.isEmpty()) err = proc.readAllStandardError().trimmed();
        qWarning() << "[ConfigValidator] checkZone failed:" << err;
        if (error) *error = err;
        return false;
    }
    return true;
}
