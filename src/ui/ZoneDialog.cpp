#include "ui/ZoneDialog.h"
#include <QDebug>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

ZoneDialog::ZoneDialog(const Zone &zone, QWidget *parent)
    : QDialog(parent) {
    qDebug() << "[ZoneDialog] открытие, zone=" << zone.name;

    setWindowTitle(zone.name.isEmpty() ? "Создать зону" : "Свойства зоны");
    setMinimumWidth(400);

    m_name = new QLineEdit(zone.name, this);

    m_type = new QComboBox(this);
    m_type->addItem("Master", static_cast<int>(ZoneType::Master));
    m_type->addItem("Slave",  static_cast<int>(ZoneType::Slave));
    m_type->setCurrentIndex(zone.type == ZoneType::Slave ? 1 : 0);

    m_filePath = new QLineEdit(zone.filePath, this);
    m_browse   = new QPushButton("Обзор...", this);

    QHBoxLayout *fileLayout = new QHBoxLayout;
    fileLayout->addWidget(m_filePath);
    fileLayout->addWidget(m_browse);

    m_masterIp = new QLineEdit(zone.masterIp, this);
    m_masterIp->setPlaceholderText("IP-адрес master-сервера");
    m_masterIp->setEnabled(zone.type == ZoneType::Slave);

    auto *form = new QFormLayout;
    form->addRow("Имя зоны:", m_name);
    form->addRow("Тип:", m_type);
    form->addRow("Файл зоны:", fileLayout);
    form->addRow("Master IP:", m_masterIp);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    auto *main = new QVBoxLayout(this);
    main->addLayout(form);
    main->addWidget(buttons);

    connect(m_type, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ZoneDialog::onTypeChanged);
    connect(m_browse,  &QPushButton::clicked,    this, &ZoneDialog::onBrowse);
    connect(m_name,    &QLineEdit::textChanged,  this, &ZoneDialog::onNameChanged);
    connect(buttons,   &QDialogButtonBox::accepted, this, &ZoneDialog::accept);
    connect(buttons,   &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void ZoneDialog::onTypeChanged(int index) {
    m_masterIp->setEnabled(index == 1); // Slave = index 1
}

void ZoneDialog::onBrowse() {
    QString path = QFileDialog::getSaveFileName(this, "Выбор файла зоны",
                                                 m_filePath->text(),
                                                 "Файлы зон (db.*);;Все файлы (*)");
    if (!path.isEmpty())
        m_filePath->setText(path);
}

void ZoneDialog::accept() {
    const QString name     = m_name->text().trimmed();
    const QString filePath = m_filePath->text().trimmed();

    if (name.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите имя зоны.");
        m_name->setFocus();
        return;
    }
    if (filePath.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Укажите путь к файлу зоны.");
        m_filePath->setFocus();
        return;
    }
    if (m_type->currentIndex() == 1 && m_masterIp->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Для slave-зоны необходимо указать IP master-сервера.");
        m_masterIp->setFocus();
        return;
    }

    QDialog::accept();
}

void ZoneDialog::onNameChanged(const QString &name) {
    const QString current = m_filePath->text();
    if (current.isEmpty() || current.startsWith("/etc/bind/db.")) {
        const QString suggested = "/etc/bind/db." + name.toLower();
        m_filePath->setText(suggested);
        qDebug() << "[ZoneDialog] autofill filePath=" << suggested;
    }
}

Zone ZoneDialog::zone() const {
    Zone z;
    z.name     = m_name->text().trimmed();
    z.type     = m_type->currentIndex() == 1 ? ZoneType::Slave : ZoneType::Master;
    z.filePath = m_filePath->text().trimmed();
    z.masterIp = m_masterIp->text().trimmed();
    z.view     = (z.name.contains("in-addr.arpa") || z.name.contains("ip6.arpa"))
                     ? ZoneView::Reverse : ZoneView::Forward;
    qDebug() << "[ZoneDialog] OK, zone=" << z.name << "type=" << (int)z.type;
    return z;
}
