#include <gtest/gtest.h>
#include <QDir>
#include <QFile>
#include <QString>
#include <QTemporaryDir>
#include <QTextStream>
#include "parser/ConfigWriter.h"
#include "parser/ZoneFileParser.h"
#include "model/Zone.h"
#include "model/ResourceRecord.h"

TEST(ConfigWriter, WriteZoneFile) {
    // Формируем зону с SOA + одной A-записью
    Zone zone;
    zone.name     = "test.example.com";
    zone.type     = ZoneType::Master;
    zone.view     = ZoneView::Forward;

    ResourceRecord soa;
    soa.name = "@";
    soa.type = RecordType::SOA;
    soa.ttl  = 3600;
    soa.data = "ns1.test.example.com. admin.test.example.com. 2026050501 3600 900 604800 300";

    ResourceRecord a;
    a.name = "www";
    a.type = RecordType::A;
    a.ttl  = 3600;
    a.data = "192.168.100.1";

    zone.records << soa << a;

    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());

    const QString destPath = tmpDir.path() + "/db.test.example.com";
    zone.filePath = destPath;

    ConfigWriter writer;
    QString error;
    bool ok = writer.writeZoneFile(zone, destPath, nullptr, &error);

    ASSERT_TRUE(ok) << "writeZoneFile вернул false: " << error.toStdString();

    // Файл существует
    EXPECT_TRUE(QFile::exists(destPath)) << "Файл зоны не создан";

    // Временный файл .tmp удалён
    EXPECT_FALSE(QFile::exists(destPath + ".tmp")) << ".tmp файл не был удалён";

    // Содержимое корректное
    QFile f(destPath);
    ASSERT_TRUE(f.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString content = f.readAll();
    f.close();

    EXPECT_TRUE(content.contains("$TTL 3600"))          << "Нет $TTL";
    EXPECT_TRUE(content.contains("@ IN SOA"))           << "Нет SOA-записи";
    EXPECT_TRUE(content.contains("www"))                << "Нет A-записи www";
    EXPECT_TRUE(content.contains("192.168.100.1"))      << "Нет IP-адреса A-записи";
}

// Вспомогательная функция: формирует минимальную обратную зону с PTR
static Zone makeReverseZone(const QString &filePath) {
    Zone zone;
    zone.name     = "1.168.192.in-addr.arpa";
    zone.type     = ZoneType::Master;
    zone.view     = ZoneView::Reverse;
    zone.filePath = filePath;

    ResourceRecord soa;
    soa.name = "@";
    soa.type = RecordType::SOA;
    soa.ttl  = 3600;
    soa.data = "ns1.example.com. admin.example.com. 2026050501 3600 900 604800 300";

    ResourceRecord ns;
    ns.name = "@";
    ns.type = RecordType::NS;
    ns.ttl  = 3600;
    ns.data = "ns1.example.com.";

    ResourceRecord ptr1;
    ptr1.name = "1";
    ptr1.type = RecordType::PTR;
    ptr1.ttl  = 3600;
    ptr1.data = "ns1.example.com.";

    ResourceRecord ptr2;
    ptr2.name = "10";
    ptr2.type = RecordType::PTR;
    ptr2.ttl  = 3600;
    ptr2.data = "host.example.com.";

    zone.records << soa << ns << ptr1 << ptr2;
    return zone;
}

TEST(ConfigWriter, WriteReverseZoneWithPTR) {
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    const QString destPath = tmpDir.path() + "/db.1.168.192.in-addr.arpa";

    Zone zone = makeReverseZone(destPath);

    ConfigWriter writer;
    QString error;
    bool ok = writer.writeZoneFile(zone, destPath, nullptr, &error);

    ASSERT_TRUE(ok) << "writeZoneFile вернул false: " << error.toStdString();
    EXPECT_FALSE(QFile::exists(destPath + ".tmp")) << ".tmp файл не удалён";

    QFile f(destPath);
    ASSERT_TRUE(f.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString content = f.readAll();
    f.close();

    EXPECT_TRUE(content.contains("$TTL 3600"))           << "Нет $TTL";
    EXPECT_TRUE(content.contains("@ IN SOA"))            << "Нет SOA";
    EXPECT_TRUE(content.contains("IN NS"))               << "Нет NS";
    EXPECT_TRUE(content.contains("IN PTR"))              << "Нет PTR";
    EXPECT_TRUE(content.contains("ns1.example.com."))    << "Нет имени хоста PTR 1";
    EXPECT_TRUE(content.contains("host.example.com."))   << "Нет имени хоста PTR 2";
    // Имена PTR (последние октеты) присутствуют как имена записей
    EXPECT_TRUE(content.contains("1 ") || content.contains("1\t")) << "Нет октета \"1\" в файле";
    EXPECT_TRUE(content.contains("10 ") || content.contains("10\t")) << "Нет октета \"10\" в файле";
}

TEST(ConfigWriter, RoundTripPTR) {
    // Записываем обратную зону и сразу парсим — PTR должны сохраниться
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    const QString destPath = tmpDir.path() + "/db.roundtrip";

    Zone zone = makeReverseZone(destPath);

    ConfigWriter writer;
    QString writeErr;
    ASSERT_TRUE(writer.writeZoneFile(zone, destPath, nullptr, &writeErr))
        << writeErr.toStdString();

    ZoneFileParser parser;
    QList<ResourceRecord> records = parser.parse(destPath);
    ASSERT_TRUE(parser.lastError().isEmpty())
        << "ZoneFileParser ошибка: " << parser.lastError().toStdString();

    int ptrCount = 0;
    bool foundNs1Ptr = false;
    bool foundHostPtr = false;
    bool nsNameCorrect = false;
    for (const ResourceRecord &rr : records) {
        if (rr.type == RecordType::PTR) {
            ++ptrCount;
            if (rr.data == "ns1.example.com.")  foundNs1Ptr  = true;
            if (rr.data == "host.example.com.") foundHostPtr = true;
        }
        // NS-запись должна иметь имя "@", а не "IN" или пустую строку
        if (rr.type == RecordType::NS && rr.name == "@")
            nsNameCorrect = true;
    }

    EXPECT_EQ(ptrCount, 2)      << "Ожидались 2 PTR-записи, получено " << ptrCount;
    EXPECT_TRUE(foundNs1Ptr)    << "PTR для ns1.example.com. не найден";
    EXPECT_TRUE(foundHostPtr)   << "PTR для host.example.com. не найден";
    EXPECT_TRUE(nsNameCorrect)  << "NS-запись имеет неверное имя после round-trip (ожидалось \"@\")";
}
