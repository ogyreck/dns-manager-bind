#include "namedconfparser.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QFileInfo>
#include <QDir>
#include <QDebug>


QList<Zone> NamedConfParser::parse(const QString &filePath) {
    QList<Zone> zones;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_lastError = "Не удалось открыть файл: " + filePath;
        qWarning() << m_lastError;
        return zones;
    }

    QString content = QTextStream(&file).readAll();
    file.close();

    // Убираем все виды комментариев
    content.remove(QRegularExpression(
        "/\\*.*?\\*/",
        QRegularExpression::DotMatchesEverythingOption));
    content.remove(QRegularExpression("//[^\n]*"));
    content.remove(QRegularExpression("#[^\n]*"));

    // --- Обрабатываем include ПЕРВЫМИ, рекурсивно ---
    // Нужно знать папку текущего файла для относительных путей
    QString currentDir = QFileInfo(filePath).absolutePath();


    QRegularExpression includeRe("include\\s+\"([^\"]+)\"\\s*;");

    auto incIt = includeRe.globalMatch(content);
    while (incIt.hasNext()) {
        auto incMatch = incIt.next();
        QString includePath = incMatch.captured(1);

        // Если путь относительный — дополняем папкой текущего файла
        if (!QFileInfo(includePath).isAbsolute()) {
            includePath = currentDir + "/" + includePath;
        }

        qDebug() << "Обрабатываем include:" << includePath;

        // Рекурсивно парсим включённый файл
        QList<Zone> subZones = parse(includePath);
        zones.append(subZones);
    }

    // --- Парсим зоны в текущем файле ---
    QRegularExpression zoneRe(
        "zone\\s+\"([^\"]+)\"\\s*(?:IN\\s*)?\\{([^}]+)\\}",
        QRegularExpression::CaseInsensitiveOption
    );

    auto it = zoneRe.globalMatch(content);
    while (it.hasNext()) {
        auto match = it.next();
        Zone zone;
        zone.name     = match.captured(1);
        QString body  = match.captured(2);

        // Парсим type
        QRegularExpression typeRe("type\\s+(\\w+)\\s*;");
        auto typeMatch = typeRe.match(body);
        if (typeMatch.hasMatch()) {
            QString t = typeMatch.captured(1).toLower();
            if      (t == "master")  zone.type = ZoneType::Master;
            else if (t == "slave")   zone.type = ZoneType::Slave;
            else if (t == "hint")    zone.type = ZoneType::Hint;
            else                     zone.type = ZoneType::Forward;
        }

        // Парсим file

        QRegularExpression fileRe("file\\s+\"([^\"]+)\"\\s*;");
        auto fileMatch = fileRe.match(body);
        if (fileMatch.hasMatch()) {
            QString zonePath = fileMatch.captured(1);
            // Тоже может быть относительным
            if (!QFileInfo(zonePath).isAbsolute()) {
                zonePath = currentDir + "/" + zonePath;
            }
            zone.filePath = zonePath;
        }

        // Парсим masters для slave-зон
        QRegularExpression mastersRe("masters\\s*\\{\\s*([\\d.]+)");
        auto mastersMatch = mastersRe.match(body);
        if (mastersMatch.hasMatch()) {
            zone.masterIp = mastersMatch.captured(1);
        }

        zones.append(zone);
        qDebug() << "Найдена зона:" << zone.name
                 << "тип:" << (int)zone.type
                 << "файл:" << zone.filePath;
    }

    return zones;
}
