#include <gtest/gtest.h>
#include <QApplication>
#include <QDebug>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QSpinBox>
#include <QTimer>
#include <QWidget>
#include "ui/RecordDialog.h"

static void scheduleDismissMessageBox() {
    QTimer::singleShot(50, []() {
        for (auto *w : QApplication::topLevelWidgets()) {
            if (auto *mb = qobject_cast<QMessageBox *>(w)) {
                qDebug() << "[test_recorddialog] закрываем QMessageBox";
                mb->close();
            }
        }
    });
}

TEST(RecordDialogTest, record_returnsCorrectValues) {
    ResourceRecord input;
    input.name     = "www";
    input.type     = RecordType::A;
    input.ttl      = 3600;
    input.data     = "192.168.1.10";
    input.priority = 0;

    RecordDialog dlg(input);
    ResourceRecord result = dlg.record();

    EXPECT_EQ(result.name,     input.name);
    EXPECT_EQ(result.type,     input.type);
    EXPECT_EQ(result.ttl,      input.ttl);
    EXPECT_EQ(result.data,     input.data);
    EXPECT_EQ(result.priority, input.priority);
}

TEST(RecordDialogTest, record_mxWithPriority) {
    ResourceRecord input;
    input.name     = "mail";
    input.type     = RecordType::MX;
    input.ttl      = 600;
    input.data     = "mail.example.com.";
    input.priority = 10;

    RecordDialog dlg(input);
    ResourceRecord result = dlg.record();

    EXPECT_EQ(result.type,     RecordType::MX);
    EXPECT_EQ(result.priority, 10);
    EXPECT_EQ(result.data,     "mail.example.com.");
}

// isVisible() возвращает false если родительский виджет не показан.
// isHidden() — точный флаг, выставляется через setVisible().
static QWidget *findPriorityRow(RecordDialog &dlg) {
    for (auto *w : dlg.findChildren<QWidget *>()) {
        if (w->findChildren<QSpinBox *>().size() == 1 &&
            w->findChildren<QLabel *>().size() == 1)
            return w;
    }
    return nullptr;
}

TEST(RecordDialogTest, priorityVisibility_mxVisible) {
    ResourceRecord mx;
    mx.type = RecordType::MX;

    RecordDialog dlg(mx);
    auto *row = findPriorityRow(dlg);
    ASSERT_NE(row, nullptr) << "Строка Priority не найдена";
    EXPECT_FALSE(row->isHidden()) << "Строка Priority должна быть видима для типа MX";
}

TEST(RecordDialogTest, priorityVisibility_aHidden) {
    ResourceRecord a;
    a.type = RecordType::A;

    RecordDialog dlg(a);
    auto *row = findPriorityRow(dlg);
    ASSERT_NE(row, nullptr) << "Строка Priority не найдена";
    EXPECT_TRUE(row->isHidden()) << "Строка Priority должна быть скрыта для типа A";
}

TEST(RecordDialogTest, emptyName_rejected) {
    ResourceRecord rr;
    rr.data = "192.168.1.1";

    RecordDialog dlg(rr);
    scheduleDismissMessageBox();
    dlg.accept();
    QApplication::processEvents();

    EXPECT_NE(dlg.result(), QDialog::Accepted);
}

TEST(RecordDialogTest, emptyData_rejected) {
    ResourceRecord rr;
    rr.name = "www";

    RecordDialog dlg(rr);
    scheduleDismissMessageBox();
    dlg.accept();
    QApplication::processEvents();

    EXPECT_NE(dlg.result(), QDialog::Accepted);
}

TEST(RecordDialogTest, validRecord_accepted) {
    ResourceRecord rr;
    rr.name = "www";
    rr.data = "192.168.1.1";
    rr.type = RecordType::A;
    rr.ttl  = 3600;

    RecordDialog dlg(rr);
    dlg.accept();

    EXPECT_EQ(dlg.result(), QDialog::Accepted);
}
