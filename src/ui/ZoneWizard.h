#pragma once
#include <QList>
#include <QPair>
#include <QWizard>
#include <QWizardPage>
#include "model/ResourceRecord.h"
#include "model/Zone.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;

// Шаг 1: имя зоны, тип, путь к файлу, IP мастера (для Slave)
class ZoneWizardPage1 : public QWizardPage {
    Q_OBJECT
public:
    explicit ZoneWizardPage1(const QString &workDir, QWidget *parent = nullptr);
    bool isComplete() const override;

private slots:
    void onNameChanged(const QString &name);
    void onTypeChanged(int index);
    void onBrowse();
    void onFilePathEdited();

private:
    QLineEdit   *m_name;
    QComboBox   *m_type;
    QLineEdit   *m_filePath;
    QPushButton *m_browse;
    QLabel      *m_masterIpLabel;
    QLineEdit   *m_masterIp;
    QString      m_workDir;
    bool         m_filePathEdited = false;
};

// Шаг 2: параметры SOA-записи
class ZoneWizardPage2 : public QWizardPage {
    Q_OBJECT
public:
    explicit ZoneWizardPage2(QWidget *parent = nullptr);
    void initializePage() override;
    bool validatePage() override;

private:
    QLineEdit *m_primaryNs;
    QLineEdit *m_adminEmail;
    QLineEdit *m_serial;
    QSpinBox  *m_defaultTtl;
    QSpinBox  *m_refresh;
    QSpinBox  *m_retry;
    QSpinBox  *m_expire;
    QSpinBox  *m_minimum;
};

// Шаг 3: NS-серверы с IP (хотя бы одна заполненная строка обязательна)
class ZoneWizardPage3 : public QWizardPage {
    Q_OBJECT
public:
    explicit ZoneWizardPage3(QWidget *parent = nullptr);
    void initializePage() override;
    bool isComplete() const override;
    QList<QPair<QString, QString>> nsEntries() const;

private slots:
    void addRow();
    void removeRow();

private:
    QTableWidget *m_table;
};

// Шаг 4: дополнительные начальные записи (необязательный)
class ZoneWizardPage4 : public QWizardPage {
    Q_OBJECT
public:
    explicit ZoneWizardPage4(QWidget *parent = nullptr);
    QList<ResourceRecord> additionalRecords() const;

private slots:
    void addRow();
    void removeRow();

private:
    QTableWidget *m_table;
};

// Главный мастер: собирает 4 страницы и строит объект Zone
class ZoneWizard : public QWizard {
    Q_OBJECT
public:
    explicit ZoneWizard(const QString &workDir, QWidget *parent = nullptr);
    Zone buildZone() const;

    // Доступ к страницам (для unit-тестов)
    ZoneWizardPage1 *wizardPage1() const { return m_page1; }
    ZoneWizardPage2 *wizardPage2() const { return m_page2; }
    ZoneWizardPage3 *wizardPage3() const { return m_page3; }
    ZoneWizardPage4 *wizardPage4() const { return m_page4; }

private:
    ZoneWizardPage1 *m_page1;
    ZoneWizardPage2 *m_page2;
    ZoneWizardPage3 *m_page3;
    ZoneWizardPage4 *m_page4;
};
