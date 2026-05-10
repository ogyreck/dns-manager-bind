#pragma once
#include <QString>
#include <QList>
#include "model/ResourceRecord.h"

enum class ZoneType { Master, Slave };
enum class ZoneView { Forward, Reverse };

struct Zone {
    QString name;
    ZoneType type = ZoneType::Master;
    ZoneView view = ZoneView::Forward;
    QString filePath;
    QString masterIp;
    QList<ResourceRecord> records;
};
