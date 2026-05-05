#pragma once
#include <QString>
#include <QList>
#include "model/ResourceRecord.h"

enum class ZoneType { Master, Slave };
enum class ZoneView { Forward, Reverse };

struct Zone {
    QString name;
    ZoneType type;
    ZoneView view;
    QString filePath;
    QString masterIp;
    QList<ResourceRecord> records;
};
