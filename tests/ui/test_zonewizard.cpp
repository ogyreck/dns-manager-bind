#include <gtest/gtest.h>
#include <QApplication>
#include <QComboBox>
#include <QDebug>
#include <QLineEdit>
#include <QTableWidget>
#include <QTableWidgetItem>
#include "ui/ZoneWizard.h"
#include "model/Zone.h"
#include "model/ResourceRecord.h"

// Вспомогательная функция: подготовить мастер с минимально валидным состоянием
// zoneName — имя зоны, nsIp — IP для первого NS-сервера
static void setupWizardState(ZoneWizard &wiz, const QString &zoneName, const QString &nsIp) {
    // Страница 1 — заполняем имя зоны
    ZoneWizardPage1 *p1 = wiz.wizardPage1();
    auto edits1 = p1->findChildren<QLineEdit *>();
    ASSERT_GE(edits1.size(), 1) << "ожидался QLineEdit для имени зоны";
    edits1[0]->setText(zoneName);
    QApplication::processEvents();

    // Страница 2 — инициализируем SOA-параметры
    ZoneWizardPage2 *p2 = wiz.wizardPage2();
    p2->initializePage();

    // Страница 3 — инициализируем, затем заполняем IP для первого NS
    ZoneWizardPage3 *p3 = wiz.wizardPage3();
    p3->initializePage();

    auto tables3 = p3->findChildren<QTableWidget *>();
    ASSERT_EQ(tables3.size(), 1);
    QTableWidget *nsTable = tables3[0];
    ASSERT_GE(nsTable->rowCount(), 1);
    nsTable->setItem(0, 1, new QTableWidgetItem(nsIp));
    QApplication::processEvents();
}

// --- buildZone() ---

TEST(ZoneWizardTest, buildZone_hasSOA) {
    ZoneWizard wiz("/etc/bind");
    setupWizardState(wiz, "example.com", "192.168.1.1");

    Zone zone = wiz.buildZone();

    bool hasSoa = false;
    for (const ResourceRecord &rr : zone.records) {
        if (rr.type == RecordType::SOA) { hasSoa = true; break; }
    }
    EXPECT_TRUE(hasSoa) << "buildZone() должен возвращать зону с SOA-записью";
}

TEST(ZoneWizardTest, buildZone_hasNS) {
    ZoneWizard wiz("/etc/bind");
    setupWizardState(wiz, "example.com", "192.168.1.1");

    Zone zone = wiz.buildZone();

    bool hasNs = false;
    for (const ResourceRecord &rr : zone.records) {
        if (rr.type == RecordType::NS) { hasNs = true; break; }
    }
    EXPECT_TRUE(hasNs) << "buildZone() должен возвращать зону с NS-записью";
}

TEST(ZoneWizardTest, buildZone_hasAForNs) {
    ZoneWizard wiz("/etc/bind");
    setupWizardState(wiz, "example.com", "10.0.0.1");

    Zone zone = wiz.buildZone();

    bool hasAWithIp = false;
    for (const ResourceRecord &rr : zone.records) {
        if (rr.type == RecordType::A && rr.data == "10.0.0.1") {
            hasAWithIp = true;
            break;
        }
    }
    EXPECT_TRUE(hasAWithIp) << "buildZone() должен создать A-запись с IP из страницы 3";
}

// --- ZoneWizardPage3 ---

TEST(ZoneWizardTest, page3_emptyIp_notComplete) {
    ZoneWizard wiz("/etc/bind");

    // Инициализируем страницы 1 и 2 чтобы field("primaryNs") был доступен
    ZoneWizardPage1 *p1 = wiz.wizardPage1();
    auto edits1 = p1->findChildren<QLineEdit *>();
    ASSERT_GE(edits1.size(), 1);
    edits1[0]->setText("example.com");
    QApplication::processEvents();

    ZoneWizardPage2 *p2 = wiz.wizardPage2();
    p2->initializePage();

    ZoneWizardPage3 *p3 = wiz.wizardPage3();
    p3->initializePage();  // добавляет строку с NS и пустым IP

    // После initializePage() NS заполнен, IP пуст → isComplete() == false
    EXPECT_FALSE(p3->isComplete())
        << "страница 3 не должна быть готова, пока IP не заполнен";
}

// --- ZoneWizardPage2 ---

TEST(ZoneWizardTest, page2_initializePage_fillsNs) {
    ZoneWizard wiz("/etc/bind");

    // Устанавливаем имя зоны в странице 1
    ZoneWizardPage1 *p1 = wiz.wizardPage1();
    auto edits1 = p1->findChildren<QLineEdit *>();
    ASSERT_GE(edits1.size(), 1);
    edits1[0]->setText("myzone.local");
    QApplication::processEvents();

    ZoneWizardPage2 *p2 = wiz.wizardPage2();
    p2->initializePage();

    // m_primaryNs — первый QLineEdit страницы 2
    auto edits2 = p2->findChildren<QLineEdit *>();
    ASSERT_GE(edits2.size(), 1);
    EXPECT_EQ(edits2[0]->text(), "ns1.myzone.local.")
        << "primaryNs должен быть ns1.{zoneName}.";
}
