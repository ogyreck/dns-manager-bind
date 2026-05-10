#include <gtest/gtest.h>
#include <QEventLoop>
#include <QFile>
#include <QProcess>
#include <QString>
#include <QTemporaryDir>
#include <QTextStream>
#include <QTimer>
#include "dns/BindManager.h"
#include "model/ResourceRecord.h"
#include "model/Zone.h"

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

// Вспомогательная функция: проверяет, что инструмент BIND9 доступен в системе.
static bool bindToolAvailable(const QString &prog) {
    QProcess p;
    p.start(prog, QStringList{});
    p.waitForFinished(3000);
    return p.error() != QProcess::FailedToStart;
}

// Вспомогательная функция: формирует минимальную зону с SOA + NS + A.
static Zone makeSimpleZone(const QString &name, const QString &filePath) {
    Zone zone;
    zone.name     = name;
    zone.type     = ZoneType::Master;
    zone.view     = ZoneView::Forward;
    zone.filePath = filePath;

    ResourceRecord soa;
    soa.name = "@"; soa.type = RecordType::SOA; soa.ttl = 3600;
    soa.data = "ns1." + name + ". admin." + name + ". 2026050501 3600 900 604800 300";

    ResourceRecord ns;
    ns.name = "@"; ns.type = RecordType::NS; ns.ttl = 3600;
    ns.data = "ns1." + name + ".";

    ResourceRecord a;
    a.name = "ns1"; a.type = RecordType::A; a.ttl = 3600; a.data = "192.168.1.1";

    zone.records << soa << ns << a;
    return zone;
}

// Проверяет, что updateNamedConf откатывает named.conf при ошибке named-checkconf.
// Тест пропускается если named-checkconf или named-checkzone не установлены.
TEST(BindManagerSaveZone, RollsBackNamedConfWhenCheckconfFails) {
    if (!bindToolAvailable("named-checkconf"))
        GTEST_SKIP() << "named-checkconf не установлен";
    if (!bindToolAvailable("named-checkzone"))
        GTEST_SKIP() << "named-checkzone не установлен";

    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());

    // named.conf с заведомо невалидным синтаксисом — checkconf провалится
    const QString confPath    = tmpDir.path() + "/named.conf";
    const QString origContent = "THIS IS INTENTIONALLY INVALID NAMED CONF SYNTAX\n";
    {
        QFile f(confPath);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream ts(&f);
        ts << origContent;
    }

    const QString zoneFile = tmpDir.path() + "/db.rollback-test.example.com";
    Zone zone = makeSimpleZone("rollback-test.example.com", zoneFile);

    BindManager mgr;
    QString error;
    const bool ok = mgr.saveZone(zone, confPath, &error);

    ASSERT_FALSE(ok) << "saveZone должен вернуть false при невалидном named.conf";
    EXPECT_FALSE(error.isEmpty()) << "Ожидалось сообщение об ошибке";
    EXPECT_TRUE(error.startsWith("named-checkconf:"))
        << "Ошибка должна содержать вывод named-checkconf, получено: " << error.toStdString();

    // Проверяем откат: named.conf должен быть неизменён
    QFile f(confPath);
    ASSERT_TRUE(f.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString curContent = f.readAll();
    f.close();
    EXPECT_EQ(curContent, origContent)
        << "named.conf должен быть восстановлен к оригинальному содержимому после ошибки checkconf";
}

// Проверяет, что commandFinished эмитируется ровно один раз при startAsync.
// Старый код мог запускать fallback-сервис, что на alias-сервисах давало
// "Start request repeated too quickly". Новый код делает один вызов.
TEST(BindManagerAsync, CommandFinishedEmittedExactlyOnce) {
    BindManager mgr;
    int emitCount = 0;
    QString capturedAction;

    QObject::connect(&mgr, &BindManager::commandFinished,
                     [&](const QString &action, bool, const QString &) {
                         ++emitCount;
                         capturedAction = action;
                     });

    mgr.startAsync();

    // Ждём первого сигнала (до 3 сек); если systemctl не ответил — пропускаем.
    // На системах без root systemctl может зависнуть ожидая polkit.
    QEventLoop waitLoop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    timeoutTimer.start(3000);
    QObject::connect(&timeoutTimer, &QTimer::timeout, &waitLoop, &QEventLoop::quit);
    QObject::connect(&mgr, &BindManager::commandFinished, &waitLoop, &QEventLoop::quit);
    waitLoop.exec();

    if (emitCount == 0)
        GTEST_SKIP() << "systemctl не ответил за 3 сек (нет прав или нет службы), тест пропущен";

    // Небольшая задержка — убеждаемся, что повторных сигналов нет
    QEventLoop drainLoop;
    QTimer::singleShot(500, &drainLoop, &QEventLoop::quit);
    drainLoop.exec();

    EXPECT_EQ(emitCount, 1) << "commandFinished должен эмитироваться ровно один раз";
    EXPECT_EQ(capturedAction.toStdString(), "start") << "Действие должно быть 'start'";
}
