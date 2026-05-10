#include <gtest/gtest.h>
#include <QString>
#include "parser/ZoneFileParser.h"

static QString fixturesDir() {
    return QString(FIXTURES_DIR);
}

TEST(ZoneFileParser, AllRecordTypes) {
    ZoneFileParser parser;
    QList<ResourceRecord> records = parser.parse(fixturesDir() + "/db.example.com");

    EXPECT_TRUE(parser.lastError().isEmpty());
    EXPECT_FALSE(records.isEmpty());

    bool hasSOA = false, hasNS = false, hasA = false, hasAAAA = false;
    bool hasMX = false, hasCNAME = false, hasTXT = false, hasPTR = false;
    int mxPriority = -1;
    QString soaData;

    for (const ResourceRecord &rr : records) {
        switch (rr.type) {
            case RecordType::SOA:   hasSOA = true; soaData = rr.data; break;
            case RecordType::NS:    hasNS  = true; break;
            case RecordType::A:     hasA   = true; break;
            case RecordType::AAAA:  hasAAAA = true; break;
            case RecordType::MX:    hasMX  = true; mxPriority = rr.priority; break;
            case RecordType::CNAME: hasCNAME = true; break;
            case RecordType::TXT:   hasTXT   = true; break;
            case RecordType::PTR:   hasPTR   = true; break;
        }
    }

    EXPECT_TRUE(hasSOA)   << "SOA запись не найдена";
    EXPECT_TRUE(hasNS)    << "NS запись не найдена";
    EXPECT_TRUE(hasA)     << "A запись не найдена";
    EXPECT_TRUE(hasAAAA)  << "AAAA запись не найдена";
    EXPECT_TRUE(hasMX)    << "MX запись не найдена";
    EXPECT_TRUE(hasCNAME) << "CNAME запись не найдена";
    EXPECT_TRUE(hasTXT)   << "TXT запись не найдена";
    EXPECT_TRUE(hasPTR)   << "PTR запись не найдена";

    EXPECT_EQ(mxPriority, 10) << "MX priority должен быть 10";
    EXPECT_FALSE(soaData.isEmpty()) << "SOA data не должна быть пустой";
}

TEST(ZoneFileParser, ReverseZone) {
    ZoneFileParser parser;
    QList<ResourceRecord> records =
        parser.parse(fixturesDir() + "/db.1.168.192.in-addr.arpa");

    EXPECT_TRUE(parser.lastError().isEmpty())
        << "Ошибка парсинга: " << parser.lastError().toStdString();
    EXPECT_FALSE(records.isEmpty());

    int ptrCount = 0;
    bool hasSOA   = false;
    bool hasNS    = false;
    QStringList ptrData;

    for (const ResourceRecord &rr : records) {
        switch (rr.type) {
            case RecordType::SOA: hasSOA = true; break;
            case RecordType::NS:  hasNS  = true; break;
            case RecordType::PTR: ++ptrCount; ptrData << rr.data; break;
            default: break;
        }
    }

    EXPECT_TRUE(hasSOA) << "SOA запись не найдена в обратной зоне";
    EXPECT_TRUE(hasNS)  << "NS запись не найдена в обратной зоне";
    EXPECT_EQ(ptrCount, 3) << "Ожидались 3 PTR-записи, получено " << ptrCount;
    EXPECT_TRUE(ptrData.contains("ns1.example.com."))  << "PTR ns1 не найден";
    EXPECT_TRUE(ptrData.contains("ns2.example.com."))  << "PTR ns2 не найден";
    EXPECT_TRUE(ptrData.contains("host.example.com.")) << "PTR host не найден";
}

// Регрессия: строка «IN NS ns1.legacy.» без owner-поля (старый формат ConfigWriter)
// должна давать NS-запись с name == "@", а не "IN".
TEST(ZoneFileParser, LegacyNsWithoutOwner) {
    ZoneFileParser parser;
    QList<ResourceRecord> records = parser.parse(fixturesDir() + "/db.legacy.ns");

    EXPECT_TRUE(parser.lastError().isEmpty());

    bool foundNs = false;
    for (const ResourceRecord &rr : records) {
        if (rr.type == RecordType::NS) {
            foundNs = true;
            EXPECT_EQ(rr.name, "@")
                << "NS без owner должен наследовать lastOwner (@), получено: "
                << rr.name.toStdString();
        }
    }
    EXPECT_TRUE(foundNs) << "NS-запись не найдена";
}

TEST(ZoneFileParser, DefaultTtl) {
    ZoneFileParser parser;
    QList<ResourceRecord> records = parser.parse(fixturesDir() + "/db.example.com");

    EXPECT_TRUE(parser.lastError().isEmpty());
    EXPECT_FALSE(records.isEmpty());

    // Все записи должны иметь TTL >= 300 (из $TTL 3600 в fixture)
    for (const ResourceRecord &rr : records) {
        EXPECT_GE(rr.ttl, 300u) << "Запись " << rr.name.toStdString()
                                << " имеет нулевой TTL";
    }
}
