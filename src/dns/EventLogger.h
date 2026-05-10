#pragma once
#include <QObject>
#include <QList>
#include "model/EventLogEntry.h"

// Синглтон-журнал событий DNS-менеджера.
// Хранит до kMaxEntries записей в памяти; при переполнении вытесняет самые старые.
class EventLogger : public QObject {
    Q_OBJECT
public:
    static EventLogger *instance();

    void log(EventLog::Level level, EventLog::Category category, const QString &message);

    QList<EventLogEntry> entries() const;

    // Возвращает записи с уровнем >= minLevel (Warning → Warning + Error, etc.)
    QList<EventLogEntry> filter(EventLog::Level minLevel) const;

    void clear();

signals:
    void eventAdded(const EventLogEntry &entry);

private:
    explicit EventLogger(QObject *parent = nullptr);

    static constexpr int kMaxEntries = 1000;
    QList<EventLogEntry> m_entries;
};
