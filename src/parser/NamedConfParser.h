#pragma once
#include <QString>
#include <QList>
#include <QSet>
#include "model/Zone.h"

class NamedConfParser {
public:
    QList<Zone> parse(const QString &filePath);
    QString lastError() const { return m_lastError; }

private:
    QList<Zone> doParse(const QString &filePath);

    QString m_lastError;
    QSet<QString> m_parsedFiles;
};
