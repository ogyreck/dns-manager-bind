#ifndef ZONE_H
#define ZONE_H

#pragma once
#include <QString>
#include <QList>

enum class ZoneType { Master, Slave, Hint, Forward };

struct Zone {
    QString name;        // "example.com"
    ZoneType type;       // Master / Slave
    QString filePath;    // "/etc/bind/db.example.com"
    QString masterIp;    // только для Slave
};

#endif // ZONE_H
