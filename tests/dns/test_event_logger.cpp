#include <gtest/gtest.h>
#include <QSignalSpy>
#include "dns/EventLogger.h"
#include "model/EventLogEntry.h"

// Вспомогательная функция: очищает синглтон перед каждым тестом.
static void resetLogger() {
    EventLogger::instance()->clear();
}

TEST(EventLogger, EventLoggerSingleton) {
    EventLogger *a = EventLogger::instance();
    EventLogger *b = EventLogger::instance();
    EXPECT_EQ(a, b);
}

TEST(EventLogger, LogAddsEntry) {
    resetLogger();
    EventLogger::instance()->log(EventLog::Level::Info,
                                 EventLog::Category::Zone,
                                 "тестовое сообщение");
    const auto entries = EventLogger::instance()->entries();
    ASSERT_EQ(entries.size(), 1);
    EXPECT_EQ(entries[0].level,    EventLog::Level::Info);
    EXPECT_EQ(entries[0].category, EventLog::Category::Zone);
    EXPECT_EQ(entries[0].message,  "тестовое сообщение");
    EXPECT_TRUE(entries[0].timestamp.isValid());
}

TEST(EventLogger, ClearRemovesEntries) {
    resetLogger();
    EventLogger::instance()->log(EventLog::Level::Debug, EventLog::Category::Server, "msg1");
    EventLogger::instance()->log(EventLog::Level::Info,  EventLog::Category::Record, "msg2");
    EXPECT_EQ(EventLogger::instance()->entries().size(), 2);

    EventLogger::instance()->clear();
    EXPECT_TRUE(EventLogger::instance()->entries().isEmpty());
}

TEST(EventLogger, FilterByLevel) {
    resetLogger();
    EventLogger::instance()->log(EventLog::Level::Debug,   EventLog::Category::Server,     "debug");
    EventLogger::instance()->log(EventLog::Level::Info,    EventLog::Category::Zone,       "info");
    EventLogger::instance()->log(EventLog::Level::Warning, EventLog::Category::Validation, "warn");
    EventLogger::instance()->log(EventLog::Level::Error,   EventLog::Category::Record,     "error");

    const auto filtered = EventLogger::instance()->filter(EventLog::Level::Warning);
    ASSERT_EQ(filtered.size(), 2);
    EXPECT_EQ(filtered[0].level, EventLog::Level::Warning);
    EXPECT_EQ(filtered[1].level, EventLog::Level::Error);
}

TEST(EventLogger, MaxCapacity) {
    resetLogger();
    for (int i = 0; i < 1001; ++i) {
        EventLogger::instance()->log(EventLog::Level::Info,
                                     EventLog::Category::Server,
                                     "msg " + QString::number(i));
    }
    EXPECT_EQ(EventLogger::instance()->entries().size(), 1000);
    // Первая запись должна быть вытеснена — проверяем что самое старое (msg 0) удалено
    EXPECT_EQ(EventLogger::instance()->entries().first().message, "msg 1");
}

TEST(EventLogger, SignalEmitted) {
    resetLogger();
    QSignalSpy spy(EventLogger::instance(), &EventLogger::eventAdded);
    EventLogger::instance()->log(EventLog::Level::Warning,
                                 EventLog::Category::Validation,
                                 "сигнальный тест");
    EXPECT_EQ(spy.count(), 1);
    const EventLogEntry emitted = qvariant_cast<EventLogEntry>(spy.at(0).at(0));
    EXPECT_EQ(emitted.message, "сигнальный тест");
    EXPECT_EQ(emitted.level,   EventLog::Level::Warning);
}
