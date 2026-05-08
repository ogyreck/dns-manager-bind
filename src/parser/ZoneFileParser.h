#pragma once
#include <QString>
#include <QList>
#include "model/ResourceRecord.h"


class ZoneFileParser {
public:

    QList<ResourceRecord> parse(const QString &filePath);
    
    QString lastError() const { return m_lastError; }

private:
    QString m_lastError; 
};
