#include <gtest/gtest.h>
#include <QString>
#include "parser/NamedConfParser.h"

static QString fixturesDir() {
    return QString(FIXTURES_DIR);
}

TEST(NamedConfParser, SimpleOneMasterZone) {
    NamedConfParser parser;
    QList<Zone> zones = parser.parse(fixturesDir() + "/named_simple.conf");

    ASSERT_EQ(zones.size(), 1);
    EXPECT_EQ(zones[0].name, "example.com");
    EXPECT_EQ(zones[0].type, ZoneType::Master);
    EXPECT_EQ(zones[0].view, ZoneView::Forward);
    EXPECT_FALSE(zones[0].filePath.isEmpty());
    EXPECT_TRUE(parser.lastError().isEmpty());
}

TEST(NamedConfParser, IncludeDirective) {
    NamedConfParser parser;
    QList<Zone> zones = parser.parse(fixturesDir() + "/named_include.conf");

    ASSERT_EQ(zones.size(), 2);

    QStringList names;
    for (const Zone &z : zones) names.append(z.name);
    EXPECT_TRUE(names.contains("main.example.com"));
    EXPECT_TRUE(names.contains("sub.example.com"));
    EXPECT_TRUE(parser.lastError().isEmpty());
}

TEST(NamedConfParser, SlaveWithMasters) {
    NamedConfParser parser;
    QList<Zone> zones = parser.parse(fixturesDir() + "/named_slave.conf");

    ASSERT_EQ(zones.size(), 2);

    Zone *slave = nullptr;
    for (Zone &z : zones)
        if (z.type == ZoneType::Slave) slave = &z;

    ASSERT_NE(slave, nullptr);
    EXPECT_EQ(slave->name, "replica.example.com");
    EXPECT_EQ(slave->masterIp, "192.168.1.100");
    EXPECT_EQ(slave->view, ZoneView::Forward);
    EXPECT_TRUE(parser.lastError().isEmpty());
}

TEST(NamedConfParser, BrokenConf) {
    NamedConfParser parser;
    QList<Zone> zones = parser.parse(fixturesDir() + "/named_broken.conf");

    EXPECT_TRUE(zones.isEmpty());
}

