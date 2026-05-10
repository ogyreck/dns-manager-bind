#include "dns/EventLogger.h"
#include <QDebug>

EventLogger *EventLogger::instance() {
    static EventLogger s_instance;
    return &s_instance;
}

EventLogger::EventLogger(QObject *parent) : QObject(parent) {
    qRegisterMetaType<EventLogEntry>("EventLogEntry");
}

void EventLogger::log(EventLog::Level level, EventLog::Category category, const QString &message) {
    EventLogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.level     = level;
    entry.category  = category;
    entry.message   = message;

    qDebug() << "[EventLogger] log: level=" << static_cast<int>(level)
             << "category=" << static_cast<int>(category)
             << "message=" << message;

    if (m_entries.size() >= kMaxEntries)
        m_entries.removeFirst();

    m_entries.append(entry);
    emit eventAdded(entry);
}

QList<EventLogEntry> EventLogger::entries() const {
    return m_entries;
}

QList<EventLogEntry> EventLogger::filter(EventLog::Level minLevel) const {
    QList<EventLogEntry> result;
    for (const EventLogEntry &e : m_entries) {
        if (static_cast<int>(e.level) >= static_cast<int>(minLevel))
            result.append(e);
    }
    return result;
}

void EventLogger::clear() {
    qDebug() << "[EventLogger] clear: removing" << m_entries.size() << "entries";
    m_entries.clear();
}
