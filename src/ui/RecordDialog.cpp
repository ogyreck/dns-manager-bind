#include "ui/RecordDialog.h"
#include <QDebug>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

static const QStringList kTypeNames = {"A", "AAAA", "NS", "MX", "CNAME", "PTR", "SOA", "TXT"};

static int typeToIndex(RecordType t) {
    switch (t) {
        case RecordType::A:     return 0;
        case RecordType::AAAA:  return 1;
        case RecordType::NS:    return 2;
        case RecordType::MX:    return 3;
        case RecordType::CNAME: return 4;
        case RecordType::PTR:   return 5;
        case RecordType::SOA:   return 6;
        case RecordType::TXT:   return 7;
    }
    return 0;
}

static RecordType indexToType(int idx) {
    static const RecordType kTypes[] = {
        RecordType::A, RecordType::AAAA, RecordType::NS, RecordType::MX,
        RecordType::CNAME, RecordType::PTR, RecordType::SOA, RecordType::TXT
    };
    if (idx >= 0 && idx < (int)(sizeof(kTypes)/sizeof(*kTypes)))
        return kTypes[idx];
    return RecordType::A;
}

RecordDialog::RecordDialog(const ResourceRecord &rr, QWidget *parent)
    : QDialog(parent) {
    qDebug() << "[RecordDialog] открытие, name=" << rr.name;

    setWindowTitle(rr.name.isEmpty() ? "Создать запись" : "Свойства записи");
    setMinimumWidth(380);

    m_name = new QLineEdit(rr.name, this);
    m_name->setPlaceholderText("@ или имя хоста");

    m_type = new QComboBox(this);
    for (const QString &t : kTypeNames) m_type->addItem(t);
    m_type->setCurrentIndex(typeToIndex(rr.type));

    m_ttl = new QSpinBox(this);
    m_ttl->setRange(0, 2147483647);
    m_ttl->setValue(static_cast<int>(rr.ttl));

    m_data = new QLineEdit(rr.data, this);

    // Поле Priority (только для MX)
    m_priority = new QSpinBox(this);
    m_priority->setRange(0, 65535);
    m_priority->setValue(rr.priority);

    m_priorityRow = new QWidget(this);
    auto *prLayout = new QHBoxLayout(m_priorityRow);
    prLayout->setContentsMargins(0, 0, 0, 0);
    prLayout->addWidget(new QLabel("Приоритет:"));
    prLayout->addWidget(m_priority);

    auto *form = new QFormLayout;
    form->addRow("Имя:", m_name);
    form->addRow("Тип:", m_type);
    form->addRow("TTL:", m_ttl);
    form->addRow("Данные:", m_data);
    form->addRow(m_priorityRow);

    // Показываем Priority только для MX
    m_priorityRow->setVisible(rr.type == RecordType::MX);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    auto *main = new QVBoxLayout(this);
    main->addLayout(form);
    main->addWidget(buttons);

    connect(m_type, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RecordDialog::onTypeChanged);
    connect(buttons, &QDialogButtonBox::accepted, this, &RecordDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void RecordDialog::onTypeChanged(int index) {
    m_priorityRow->setVisible(index == 3); // MX = index 3
}

void RecordDialog::accept() {
    const QString name = m_name->text().trimmed();
    const QString data = m_data->text().trimmed();
    qDebug() << "[RecordDialog] accept attempt, name=" << name << "data=" << data;

    if (name.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите имя записи.");
        m_name->setFocus();
        return;
    }
    if (data.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите значение записи (поле «Данные»).");
        m_data->setFocus();
        return;
    }

    QDialog::accept();
}

ResourceRecord RecordDialog::record() const {
    ResourceRecord rr;
    rr.name     = m_name->text().trimmed();
    rr.type     = indexToType(m_type->currentIndex());
    rr.ttl      = static_cast<uint32_t>(m_ttl->value());
    rr.data     = m_data->text().trimmed();
    rr.priority = m_priority->value();
    qDebug() << "[RecordDialog] OK, name=" << rr.name << "type=" << m_type->currentText();
    return rr;
}
