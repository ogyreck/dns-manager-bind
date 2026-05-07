#include "ui/SettingsDialog.h"
#include <QDebug>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(const ServerConfig &cfg, QWidget *parent)
    : QDialog(parent)
{
    qDebug() << "[SettingsDialog] открытие, namedConfPath=" << cfg.namedConfPath
             << "workDir=" << cfg.workDir;

    setWindowTitle("Настройки сервера");
    setMinimumWidth(450);

    m_namedConfPath = new QLineEdit(cfg.namedConfPath, this);
    auto *btnBrowseConf = new QPushButton("Обзор...", this);

    m_workDir = new QLineEdit(cfg.workDir, this);
    auto *btnBrowseWork = new QPushButton("Обзор...", this);

    auto *confLayout = new QHBoxLayout;
    confLayout->addWidget(m_namedConfPath);
    confLayout->addWidget(btnBrowseConf);

    auto *workLayout = new QHBoxLayout;
    workLayout->addWidget(m_workDir);
    workLayout->addWidget(btnBrowseWork);

    auto *form = new QFormLayout;
    form->addRow("Путь к named.conf:", confLayout);
    form->addRow("Рабочий каталог:", workLayout);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    auto *main = new QVBoxLayout(this);
    main->addLayout(form);
    main->addWidget(buttons);

    connect(btnBrowseConf, &QPushButton::clicked, this, &SettingsDialog::onBrowseNamedConf);
    connect(btnBrowseWork, &QPushButton::clicked, this, &SettingsDialog::onBrowseWorkDir);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void SettingsDialog::onBrowseNamedConf() {
    const QString path = QFileDialog::getOpenFileName(
        this, "Выбор named.conf", m_namedConfPath->text(), "Конфиги (named.conf);;Все файлы (*)");
    if (!path.isEmpty()) {
        m_namedConfPath->setText(path);
        qDebug() << "[SettingsDialog] namedConfPath выбран:" << path;
    }
}

void SettingsDialog::onBrowseWorkDir() {
    const QString path = QFileDialog::getExistingDirectory(
        this, "Выбор рабочего каталога BIND", m_workDir->text());
    if (!path.isEmpty()) {
        m_workDir->setText(path);
        qDebug() << "[SettingsDialog] workDir выбран:" << path;
    }
}

ServerConfig SettingsDialog::config() const {
    ServerConfig cfg;
    cfg.namedConfPath = m_namedConfPath->text().trimmed();
    cfg.workDir       = m_workDir->text().trimmed();
    qDebug() << "[SettingsDialog] OK: namedConfPath=" << cfg.namedConfPath
             << "workDir=" << cfg.workDir;
    return cfg;
}
