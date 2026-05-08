#include <gtest/gtest.h>
#include <QString>
#include "dns/BindManager.h"

static QString fixturesDir() {
    return QString(FIXTURES_DIR);
}

TEST(BindManager, LoadZones) {
    BindManager mgr;
    QString error;
    QList<Zone> zones = mgr.loadZones(fixturesDir() + "/named_simple.conf", &error);

    EXPECT_TRUE(error.isEmpty()) << error.toStdString();
    EXPECT_FALSE(zones.isEmpty()) << "Должна быть хотя бы одна зона";
}

TEST(BindManager, LoadZoneRecords) {
    BindManager mgr;
    QString error;
    QList<ResourceRecord> records = mgr.loadZoneRecords(fixturesDir() + "/db.example.com", &error);

    EXPECT_TRUE(error.isEmpty()) << error.toStdString();
    EXPECT_FALSE(records.isEmpty());

    bool hasSOA = false, hasA = false;
    for (const ResourceRecord &rr : records) {
        if (rr.type == RecordType::SOA) hasSOA = true;
        if (rr.type == RecordType::A)   hasA   = true;
    }
    EXPECT_TRUE(hasSOA) << "SOA запись не найдена";
    EXPECT_TRUE(hasA)   << "A запись не найдена";
}

TEST(BindManager, LoadZonesError) {
    BindManager mgr;
    QString error;
    QList<Zone> zones = mgr.loadZones("/nonexistent/path/named.conf", &error);

    EXPECT_FALSE(error.isEmpty()) << "Ожидалась ошибка для несуществующего файла";
    EXPECT_TRUE(zones.isEmpty())  << "Список зон должен быть пустым при ошибке";
}
