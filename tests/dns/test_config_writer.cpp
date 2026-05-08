#include <gtest/gtest.h>
#include <QDir>
#include <QFile>
#include <QString>
#include <QTemporaryDir>
#include <QTextStream>
#include "parser/ConfigWriter.h"
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
