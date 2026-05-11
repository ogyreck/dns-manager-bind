#pragma once
#include <QDateTime>
#include <QMetaType>
#include <QString>

namespace EventLog {

enum class Level {
    Debug,
    Info,
    Warning,
    Error
};

enum class Category {
    Server,
    Zone,
    Record,
    Validation
};

} // namespace EventLog

struct EventLogEntry {
    QDateTime         timestamp;
    EventLog::Level   level;
    EventLog::Category category;
    QString           message;
};

Q_DECLARE_METATYPE(EventLogEntry)
