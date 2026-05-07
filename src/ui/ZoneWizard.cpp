#include "ui/ZoneWizard.h"

#include <QComboBox>
#include <QDate>
#include <QDebug>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

// =============================================================================
// ZoneWizardPage1 — базовая информация зоны
// =============================================================================

ZoneWizardPage1::ZoneWizardPage1(const QString &workDir, QWidget *parent)
    : QWizardPage(parent), m_workDir(workDir)
{
    setTitle("Основная информация");
    setSubTitle("Введите имя, тип и путь к файлу зоны.");

    m_name = new QLineEdit(this);
    m_name->setPlaceholderText("example.com");

    m_type = new QComboBox(this);
    m_type->addItem("Master", static_cast<int>(ZoneType::Master));
    m_type->addItem("Slave",  static_cast<int>(ZoneType::Slave));

    m_filePath = new QLineEdit(this);
    m_filePath->setPlaceholderText(workDir + "/db.example.com");
    m_browse = new QPushButton("Обзор...", this);

    auto *fileLayout = new QHBoxLayout;
    fileLayout->addWidget(m_filePath);
    fileLayout->addWidget(m_browse);

    m_masterIpLabel = new QLabel("Master IP:", this);
    m_masterIp = new QLineEdit(this);
    m_masterIp->setPlaceholderText("192.168.1.1");
    m_masterIpLabel->setVisible(false);
    m_masterIp->setVisible(false);

    auto *form = new QFormLayout(this);
    form->addRow("Имя зоны:", m_name);
    form->addRow("Тип:", m_type);
    form->addRow("Файл зоны:", fileLayout);
    form->addRow(m_masterIpLabel, m_masterIp);

    // Регистрируем поля для межстраничного доступа через field()
    registerField("zoneName",  m_name);
    registerField("filePath",  m_filePath);
    registerField("masterIp",  m_masterIp);
    registerField("zoneType",  m_type, "currentIndex",
                  SIGNAL(currentIndexChanged(int)));

    connect(m_name,     &QLineEdit::textChanged,
            this,       &ZoneWizardPage1::onNameChanged);
    connect(m_name,     &QLineEdit::textChanged,
            this,       &QWizardPage::completeChanged);
    connect(m_filePath, &QLineEdit::textChanged,
            this,       &QWizardPage::completeChanged);
    connect(m_filePath, &QLineEdit::textEdited,
            this,       &ZoneWizardPage1::onFilePathEdited);
    connect(m_masterIp, &QLineEdit::textChanged,
            this,       &QWizardPage::completeChanged);
    connect(m_type,     QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,       &ZoneWizardPage1::onTypeChanged);
    connect(m_browse,   &QPushButton::clicked,
            this,       &ZoneWizardPage1::onBrowse);
}

bool ZoneWizardPage1::isComplete() const {
    const bool nameOk = !m_name->text().trimmed().isEmpty();
    const bool pathOk = !m_filePath->text().trimmed().isEmpty();
    const bool slaveOk = m_type->currentIndex() == 0
                         || !m_masterIp->text().trimmed().isEmpty();
    const bool result = nameOk && pathOk && slaveOk;
    qDebug() << "[ZoneWizardPage1] isComplete:" << result
             << "name=" << m_name->text()
             << "filePath=" << m_filePath->text();
    return result;
}

void ZoneWizardPage1::onNameChanged(const QString &name) {
    const QString current = m_filePath->text();
    const QString prefix  = m_workDir + "/db.";
    if (!m_filePathEdited && (current.isEmpty() || current.startsWith(prefix))) {
        const QString suggested = prefix + name.toLower();
        m_filePath->setText(suggested);
        qDebug() << "[ZoneWizardPage1] auto filePath=" << suggested;
    }
}

void ZoneWizardPage1::onTypeChanged(int index) {
    const bool isSlave = (index == 1);
    m_masterIpLabel->setVisible(isSlave);
    m_masterIp->setVisible(isSlave);
    emit completeChanged();
}

void ZoneWizardPage1::onBrowse() {
    const QString path = QFileDialog::getSaveFileName(
        this, "Выбор файла зоны", m_filePath->text(),
        "Файлы зон (db.*);;Все файлы (*)");
    if (!path.isEmpty()) {
        m_filePath->setText(path);
        m_filePathEdited = true;
    }
}

void ZoneWizardPage1::onFilePathEdited() {
    m_filePathEdited = true;
}

// =============================================================================
// ZoneWizardPage2 — конфигурация SOA
// =============================================================================

ZoneWizardPage2::ZoneWizardPage2(QWidget *parent) : QWizardPage(parent)
{
    setTitle("Конфигурация SOA");
    setSubTitle("Параметры зоны ответственности (Start of Authority).");

    m_primaryNs  = new QLineEdit(this);
    m_adminEmail = new QLineEdit(this);

    m_serial = new QLineEdit(this);
    m_serial->setReadOnly(true);

    m_defaultTtl = new QSpinBox(this);
    m_defaultTtl->setRange(300, 86400);
    m_defaultTtl->setValue(3600);
    m_defaultTtl->setSuffix(" с");

    m_refresh = new QSpinBox(this);
    m_refresh->setRange(60, 86400);
    m_refresh->setValue(3600);
    m_refresh->setSuffix(" с");

    m_retry = new QSpinBox(this);
    m_retry->setRange(60, 86400);
    m_retry->setValue(900);
    m_retry->setSuffix(" с");

    m_expire = new QSpinBox(this);
    m_expire->setRange(3600, 2592000);
    m_expire->setValue(604800);
    m_expire->setSuffix(" с");

    m_minimum = new QSpinBox(this);
    m_minimum->setRange(60, 86400);
    m_minimum->setValue(300);
    m_minimum->setSuffix(" с");

    auto *form = new QFormLayout(this);
    form->addRow("Primary NS:", m_primaryNs);
    form->addRow("Admin email:", m_adminEmail);
    form->addRow("Serial:", m_serial);
    form->addRow("Default TTL:", m_defaultTtl);
    form->addRow("Refresh:", m_refresh);
    form->addRow("Retry:", m_retry);
    form->addRow("Expire:", m_expire);
    form->addRow("Minimum TTL:", m_minimum);

    registerField("primaryNs*",  m_primaryNs);
    registerField("adminEmail",  m_adminEmail);
    registerField("serial",      m_serial);
    registerField("defaultTtl",  m_defaultTtl, "value", SIGNAL(valueChanged(int)));
    registerField("soaRefresh",  m_refresh,    "value", SIGNAL(valueChanged(int)));
    registerField("soaRetry",    m_retry,      "value", SIGNAL(valueChanged(int)));
    registerField("soaExpire",   m_expire,     "value", SIGNAL(valueChanged(int)));
    registerField("soaMinimum",  m_minimum,    "value", SIGNAL(valueChanged(int)));
}

void ZoneWizardPage2::initializePage() {
    const QString zoneName = field("zoneName").toString();
    m_primaryNs->setText("ns1." + zoneName + ".");
    m_adminEmail->setText("admin." + zoneName + ".");

    const QString serial = QDate::currentDate().toString("yyyyMMdd") + "01";
    m_serial->setText(serial);

    qDebug() << "[ZoneWizardPage2] initializePage: primaryNs=" << m_primaryNs->text()
             << "serial=" << serial;
}

bool ZoneWizardPage2::validatePage() {
    const QString ns = m_primaryNs->text().trimmed();
    if (ns.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Поле Primary NS не может быть пустым.");
        m_primaryNs->setFocus();
        return false;
    }
    if (!ns.endsWith('.')) {
        QMessageBox::warning(this, "Ошибка",
            "Primary NS должен заканчиваться точкой (например: ns1.example.com.)");
        m_primaryNs->setFocus();
        return false;
    }
    qDebug() << "[ZoneWizardPage2] validatePage: ok, primaryNs=" << ns;
    return true;
}

// =============================================================================
// ZoneWizardPage3 — серверы имён (NS + IP)
// =============================================================================

ZoneWizardPage3::ZoneWizardPage3(QWidget *parent) : QWizardPage(parent)
{
    setTitle("Серверы имён");
    setSubTitle("Укажите NS-серверы и их IP-адреса. Хотя бы одна строка обязательна.");

    m_table = new QTableWidget(0, 2, this);
    m_table->setHorizontalHeaderLabels({"NS-сервер", "IP-адрес"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    auto *addBtn = new QPushButton("Добавить строку", this);
    auto *delBtn = new QPushButton("Удалить строку", this);

    auto *btnLayout = new QHBoxLayout;
    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(delBtn);
    btnLayout->addStretch();

    auto *vbox = new QVBoxLayout(this);
    vbox->addWidget(m_table);
    vbox->addLayout(btnLayout);

    connect(addBtn, &QPushButton::clicked, this, &ZoneWizardPage3::addRow);
    connect(delBtn, &QPushButton::clicked, this, &ZoneWizardPage3::removeRow);
    connect(m_table, &QTableWidget::itemChanged, this,
            [this](QTableWidgetItem *) { emit completeChanged(); });
}

void ZoneWizardPage3::initializePage() {
    m_table->blockSignals(true);
    m_table->setRowCount(0);
    m_table->insertRow(0);
    m_table->setItem(0, 0, new QTableWidgetItem(field("primaryNs").toString()));
    m_table->setItem(0, 1, new QTableWidgetItem(""));
    m_table->blockSignals(false);

    qDebug() << "[ZoneWizardPage3] initializePage: prefilled NS="
             << field("primaryNs").toString();
    emit completeChanged();
}

bool ZoneWizardPage3::isComplete() const {
    const auto entries = nsEntries();
    const bool complete = !entries.isEmpty();
    qDebug() << "[ZoneWizardPage3] isComplete:" << complete
             << "entries=" << entries.size();
    return complete;
}

QList<QPair<QString, QString>> ZoneWizardPage3::nsEntries() const {
    QList<QPair<QString, QString>> result;
    for (int row = 0; row < m_table->rowCount(); ++row) {
        const auto *nsItem = m_table->item(row, 0);
        const auto *ipItem = m_table->item(row, 1);
        if (!nsItem || !ipItem) continue;
        const QString ns = nsItem->text().trimmed();
        const QString ip = ipItem->text().trimmed();
        if (!ns.isEmpty() && !ip.isEmpty())
            result.append({ns, ip});
    }
    return result;
}

void ZoneWizardPage3::addRow() {
    const int row = m_table->rowCount();
    m_table->insertRow(row);
    m_table->setItem(row, 0, new QTableWidgetItem(""));
    m_table->setItem(row, 1, new QTableWidgetItem(""));
    qDebug() << "[ZoneWizardPage3] row added, total=" << m_table->rowCount();
    emit completeChanged();
}

void ZoneWizardPage3::removeRow() {
    const int row = m_table->currentRow();
    if (row >= 0) {
        m_table->removeRow(row);
        qDebug() << "[ZoneWizardPage3] row removed, total=" << m_table->rowCount();
        emit completeChanged();
    }
}

// =============================================================================
// ZoneWizardPage4 — начальные записи (необязательная)
// =============================================================================

ZoneWizardPage4::ZoneWizardPage4(QWidget *parent) : QWizardPage(parent)
{
    setTitle("Начальные записи (необязательно)");
    setSubTitle("Добавьте дополнительные записи зоны или перейдите к завершению.");

    m_table = new QTableWidget(0, 3, this);
    m_table->setHorizontalHeaderLabels({"Имя", "Тип", "Значение"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    auto *addBtn = new QPushButton("Добавить", this);
    auto *delBtn = new QPushButton("Удалить", this);

    auto *btnLayout = new QHBoxLayout;
    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(delBtn);
    btnLayout->addStretch();

    auto *vbox = new QVBoxLayout(this);
    vbox->addWidget(m_table);
    vbox->addLayout(btnLayout);

    connect(addBtn, &QPushButton::clicked, this, &ZoneWizardPage4::addRow);
    connect(delBtn, &QPushButton::clicked, this, &ZoneWizardPage4::removeRow);
}

static RecordType recordTypeFromString(const QString &s) {
    if (s == "AAAA")  return RecordType::AAAA;
    if (s == "CNAME") return RecordType::CNAME;
    if (s == "MX")    return RecordType::MX;
    if (s == "TXT")   return RecordType::TXT;
    return RecordType::A;
}

QList<ResourceRecord> ZoneWizardPage4::additionalRecords() const {
    QList<ResourceRecord> result;
    for (int row = 0; row < m_table->rowCount(); ++row) {
        const auto *nameItem  = m_table->item(row, 0);
        const auto *typeCombo = qobject_cast<QComboBox *>(m_table->cellWidget(row, 1));
        const auto *valueItem = m_table->item(row, 2);
        if (!nameItem || !typeCombo || !valueItem) continue;
        const QString name  = nameItem->text().trimmed();
        const QString type  = typeCombo->currentText();
        const QString value = valueItem->text().trimmed();
        if (name.isEmpty() || value.isEmpty()) continue;

        ResourceRecord rr;
        rr.name = name;
        rr.type = recordTypeFromString(type);
        rr.ttl  = 3600;
        rr.data = value;
        qDebug() << "[ZoneWizardPage4] record collected: name=" << name
                 << "type=" << type << "value=" << value;
        result.append(rr);
    }
    return result;
}

void ZoneWizardPage4::addRow() {
    const int row = m_table->rowCount();
    m_table->insertRow(row);
    m_table->setItem(row, 0, new QTableWidgetItem(""));

    auto *typeCombo = new QComboBox;
    typeCombo->addItems({"A", "AAAA", "CNAME", "MX", "TXT"});
    m_table->setCellWidget(row, 1, typeCombo);

    m_table->setItem(row, 2, new QTableWidgetItem(""));
    qDebug() << "[ZoneWizardPage4] record added: row=" << row;
}

void ZoneWizardPage4::removeRow() {
    const int row = m_table->currentRow();
    if (row >= 0) {
        m_table->removeRow(row);
        qDebug() << "[ZoneWizardPage4] record removed, total=" << m_table->rowCount();
    }
}

// =============================================================================
// ZoneWizard — главный класс, собирает все страницы
// =============================================================================

ZoneWizard::ZoneWizard(const QString &workDir, QWidget *parent)
    : QWizard(parent)
{
    setWindowTitle("Создание DNS-зоны");
    setMinimumSize(500, 400);

    m_page1 = new ZoneWizardPage1(workDir, this);
    m_page2 = new ZoneWizardPage2(this);
    m_page3 = new ZoneWizardPage3(this);
    m_page4 = new ZoneWizardPage4(this);

    addPage(m_page1);
    addPage(m_page2);
    addPage(m_page3);
    addPage(m_page4);
}

Zone ZoneWizard::buildZone() const {
    Zone zone;
    zone.name     = field("zoneName").toString().trimmed();
    zone.type     = field("zoneType").toInt() == 1 ? ZoneType::Slave : ZoneType::Master;
    zone.filePath = field("filePath").toString().trimmed();
    zone.masterIp = field("masterIp").toString().trimmed();
    zone.view     = (zone.name.contains("in-addr.arpa") || zone.name.contains("ip6.arpa"))
                        ? ZoneView::Reverse : ZoneView::Forward;

    // SOA-запись
    ResourceRecord soa;
    soa.type = RecordType::SOA;
    soa.name = "@";
    soa.ttl  = field("defaultTtl").toInt();
    soa.data = QString("%1 %2 ( %3 %4 %5 %6 %7 )")
                   .arg(field("primaryNs").toString())
                   .arg(field("adminEmail").toString())
                   .arg(field("serial").toString())
                   .arg(field("soaRefresh").toInt())
                   .arg(field("soaRetry").toInt())
                   .arg(field("soaExpire").toInt())
                   .arg(field("soaMinimum").toInt());
    zone.records.append(soa);

    // NS и A-записи из страницы 3
    const int ttl = field("defaultTtl").toInt();
    for (const auto &entry : m_page3->nsEntries()) {
        const QString ns = entry.first;
        const QString ip = entry.second;

        ResourceRecord nsRr;
        nsRr.type = RecordType::NS;
        nsRr.name = "@";
        nsRr.ttl  = ttl;
        nsRr.data = ns;
        zone.records.append(nsRr);

        // A-запись для NS: nsShortName = ns без ".zoneName." суффикса
        QString nsShortName = ns;
        if (nsShortName.endsWith('.'))
            nsShortName.chop(1);
        const QString zoneSuffix = "." + zone.name;
        if (nsShortName.endsWith(zoneSuffix))
            nsShortName.chop(zoneSuffix.length());

        ResourceRecord aRr;
        aRr.type = RecordType::A;
        aRr.name = nsShortName;
        aRr.ttl  = ttl;
        aRr.data = ip;
        zone.records.append(aRr);
    }

    // Дополнительные записи из страницы 4
    for (const ResourceRecord &rr : m_page4->additionalRecords())
        zone.records.append(rr);

    qDebug() << "[ZoneWizard] buildZone: zone=" << zone.name
             << "records=" << zone.records.size();
    return zone;
}
