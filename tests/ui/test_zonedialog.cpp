#include <gtest/gtest.h>
#include <QApplication>
#include <QComboBox>
#include <QDebug>
#include <QLineEdit>
#include <QMessageBox>
#include <QTimer>
#include "ui/ZoneDialog.h"

// Закрыть первый QMessageBox через 50 мс — нужно для тестов с невалидными данными
static void scheduleDismissMessageBox() {
    QTimer::singleShot(50, []() {
        for (auto *w : QApplication::topLevelWidgets()) {
            if (auto *mb = qobject_cast<QMessageBox *>(w)) {
                qDebug() << "[test_zonedialog] закрываем QMessageBox";
                mb->close();
            }
        }
    });
}

// Вернуть QLineEdit диалога по порядку их создания (m_name, m_filePath, m_masterIp)
static QList<QLineEdit *> zoneEdits(ZoneDialog &dlg) {
    return dlg.findChildren<QLineEdit *>();
}

TEST(ZoneDialogTest, zone_returnsCorrectValues) {
    Zone input;
    input.name     = "example.com";
    input.type     = ZoneType::Master;
    input.filePath = "/etc/bind/db.example.com";
    input.masterIp = "";

    ZoneDialog dlg(input);
    Zone result = dlg.zone();

    EXPECT_EQ(result.name,     input.name);
    EXPECT_EQ(result.filePath, input.filePath);
    EXPECT_EQ(result.type,     input.type);
    EXPECT_EQ(result.view,     ZoneView::Forward);
}

TEST(ZoneDialogTest, zone_reverseViewDetection) {
    Zone input;
    input.name     = "1.168.192.in-addr.arpa";
    input.filePath = "/etc/bind/db.1.168.192";

    ZoneDialog dlg(input);
    Zone result = dlg.zone();

    EXPECT_EQ(result.view, ZoneView::Reverse);
}

TEST(ZoneDialogTest, autoFillPath) {
    ZoneDialog dlg(Zone{});

    auto edits = zoneEdits(dlg);
    ASSERT_GE(edits.size(), 2) << "Ожидались минимум 2 QLineEdit (name, filePath)";

    // edits[0] — m_name, edits[1] — m_filePath
    edits[0]->setText("testzone.local");
    QApplication::processEvents();

    EXPECT_EQ(dlg.zone().filePath, "/etc/bind/db.testzone.local");
}

TEST(ZoneDialogTest, autoFillPath_doesNotOverrideManual) {
    ZoneDialog dlg(Zone{});
    auto edits = zoneEdits(dlg);
    ASSERT_GE(edits.size(), 2);

    // Вручную вводим путь, не начинающийся с /etc/bind/db.
    edits[1]->setText("/custom/path/db.zone");
    edits[0]->setText("anyname.com");
    QApplication::processEvents();

    // Автозаполнение не должно перетереть ручной путь
    EXPECT_EQ(dlg.zone().filePath, "/custom/path/db.zone");
}

TEST(ZoneDialogTest, emptyName_rejected) {
    ZoneDialog dlg(Zone{});
    // name пустое — автозаполненный filePath уберём для чистоты
    auto edits = zoneEdits(dlg);
    if (!edits.isEmpty()) edits[0]->setText("");
    if (edits.size() >= 2) edits[1]->setText("/etc/bind/db.tmp");

    scheduleDismissMessageBox();
    dlg.accept();
    QApplication::processEvents();

    EXPECT_NE(dlg.result(), QDialog::Accepted);
}

TEST(ZoneDialogTest, emptyFilePath_rejected) {
    Zone zone;
    zone.name     = "example.com";
    zone.filePath = "";

    ZoneDialog dlg(zone);
    auto edits = zoneEdits(dlg);
    // Очистить filePath (автозаполнение могло поставить значение)
    for (auto *e : edits) {
        if (e->text().startsWith("/etc/bind/db."))
            e->clear();
    }

    scheduleDismissMessageBox();
    dlg.accept();
    QApplication::processEvents();

    EXPECT_NE(dlg.result(), QDialog::Accepted);
}

TEST(ZoneDialogTest, slaveMissingIp_rejected) {
    Zone zone;
    zone.name     = "example.com";
    zone.filePath = "/etc/bind/db.example.com";
    zone.type     = ZoneType::Slave;
    zone.masterIp = "";

    ZoneDialog dlg(zone);
    scheduleDismissMessageBox();
    dlg.accept();
    QApplication::processEvents();

    EXPECT_NE(dlg.result(), QDialog::Accepted);
}

TEST(ZoneDialogTest, validMaster_accepted) {
    Zone zone;
    zone.name     = "example.com";
    zone.filePath = "/etc/bind/db.example.com";
    zone.type     = ZoneType::Master;

    ZoneDialog dlg(zone);
    dlg.accept();

    EXPECT_EQ(dlg.result(), QDialog::Accepted);
}
