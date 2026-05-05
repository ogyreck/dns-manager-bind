#ifndef NAMEDCONFPARSER_H
#define NAMEDCONFPARSER_H
#pragma once
#include <QString>
#include <QList>
#include "Zone.h"

class NamedConfParser {
public:
    // Метод, для получния списка зон
    // На вход - путь до файла, возращает - список зон
    QList<Zone> parse(const QString &filePath);

    QString lastError() const { return m_lastError; }

private:
    QString m_lastError;
};

#endif // NAMEDCONFPARSER_H
