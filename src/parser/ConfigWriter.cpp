#include "parser/ConfigWriter.h"
#include "dns/ConfigValidator.h"
#include "model/ResourceRecord.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>

static QString recordTypeToStr(RecordType t) {
    switch (t) {
        case RecordType::A:     return "A";
        case RecordType::AAAA:  return "AAAA";
        case RecordType::NS:    return "NS";
        case RecordType::MX:    return "MX";
        case RecordType::CNAME: return "CNAME";
        case RecordType::PTR:   return "PTR";
        case RecordType::SOA:   return "SOA";
        case RecordType::TXT:   return "TXT";
    }
    return "?";
}

bool ConfigWriter::writeZoneFile(const Zone &zone,
                                  const QString &destPath,
                                  ConfigValidator *validator,
                                  QString *error) {
    const QString tmpPath = destPath + ".tmp";
    qDebug() << "[ConfigWriter] writeZoneFile zone=" << zone.name << "dest=" << destPath;

    // 1. Формируем текст зоны
    QString content;
    QTextStream out(&content);

    // Найти SOA запись для $TTL
    const ResourceRecord *soaRec = nullptr;
    for (const ResourceRecord &rr : zone.records) {
        if (rr.type == RecordType::SOA) { soaRec = &rr; break; }
    }

    uint32_t defaultTtl = soaRec ? soaRec->ttl : 3600;
    out << "$TTL " << defaultTtl << "\n";

    if (soaRec) {
        out << "@ IN SOA " << soaRec->data << "\n";
    }

    // NS-записи
    for (const ResourceRecord &rr : zone.records) {
        if (rr.type == RecordType::NS) {
            out << rr.name << " IN NS " << rr.data << "\n";
        }
    }

    // Остальные записи (кроме SOA и NS)
    for (const ResourceRecord &rr : zone.records) {
        if (rr.type == RecordType::SOA || rr.type == RecordType::NS)
            continue;

        QString ttlStr = rr.ttl ? QString::number(rr.ttl) : "";
        QString dataStr;
        if (rr.type == RecordType::MX)
            dataStr = QString::number(rr.priority) + " " + rr.data;
        else
            dataStr = rr.data;

        out << rr.name;
        if (!ttlStr.isEmpty()) out << " " << ttlStr;
        out << " IN " << recordTypeToStr(rr.type) << " " << dataStr << "\n";
    }

    // 2. Запись во временный файл
    qDebug() << "[ConfigWriter] writing tmp file:" << tmpPath;
    QFile tmpFile(tmpPath);
    if (!tmpFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QString err = "Не удалось открыть для записи: " + tmpPath;
        qWarning() << "[ConfigWriter]" << err;
        if (error) *error = err;
        return false;
    }
    QTextStream fileOut(&tmpFile);
    fileOut << content;
    tmpFile.close();
    qDebug() << "[ConfigWriter] tmp file written";

    // 3. Валидация (если validator задан)
    if (validator) {
        qDebug() << "[ConfigWriter] validating zone:" << zone.name;
        QString valErr;
        if (!validator->checkZone(zone.name, tmpPath, &valErr)) {
            qWarning() << "[ConfigWriter] validation failed:" << valErr;
            tmpFile.remove();
            if (error) *error = valErr;
            return false;
        }
        qDebug() << "[ConfigWriter] validation passed";
    }

    // 4. Атомарное переименование
    if (QFile::exists(destPath) && !QFile::remove(destPath)) {
        QString err = "Не удалось удалить старый файл: " + destPath;
        qWarning() << "[ConfigWriter]" << err;
        tmpFile.remove();
        if (error) *error = err;
        return false;
    }
    if (!QFile::rename(tmpPath, destPath)) {
        QString err = "Не удалось переименовать " + tmpPath + " в " + destPath;
        qWarning() << "[ConfigWriter]" << err;
        if (error) *error = err;
        return false;
    }

    qDebug() << "[ConfigWriter] done, written to:" << destPath;
    return true;
}
