#include "ui/MainWindow.h"
#include "ui/ZoneDialog.h"
#include "ui/RecordDialog.h"
#include "ui/SettingsDialog.h"
#include "ui_mainwindow.h"

#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <QSettings>
#include <QStyle>
#include <QTimer>

static QString recordTypeToString(RecordType t) {
    switch (t) {
        case RecordType::A:     return "A";
        case RecordType::AAAA:  return "AAAA";
        case RecordType::NS:    return "NS";
        case RecordType::MX:    return "MX";
        case RecordType::CNAME: return "CNAME";
        case RecordType::PTR:   return "PTR";
        case RecordType::SOA:   return "SOA";
        case RecordType::TXT:   return "TXT";
    }
    return "?";
}



MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
    m_bindManager(new BindManager(this))
{
    setWindowTitle("dns-manager-admin");
    setMinimumSize(900, 600);
    resize(1100, 700);

    connect(m_bindManager, &BindManager::commandFinished,
            this, &MainWindow::onServerCommandFinished);

    loadSettings();
    setupToolBar();
    setupCentralWidget();
    setupStatusBar();
    populateTree();
    updateActions();
}

void MainWindow::loadSettings() {
    QSettings s("dns-manager-admin", "dns-manager-admin");
    m_serverConfig.namedConfPath = s.value("server/namedConfPath",
                                            m_serverConfig.namedConfPath).toString();
    m_serverConfig.workDir       = s.value("server/workDir",
                                            m_serverConfig.workDir).toString();
    qDebug() << "[MainWindow] loadSettings: namedConfPath=" << m_serverConfig.namedConfPath
             << "workDir=" << m_serverConfig.workDir;
}

void MainWindow::saveSettings() {
    QSettings s("dns-manager-admin", "dns-manager-admin");
    s.setValue("server/namedConfPath", m_serverConfig.namedConfPath);
    s.setValue("server/workDir",       m_serverConfig.workDir);
    qDebug() << "[MainWindow] saveSettings: namedConfPath=" << m_serverConfig.namedConfPath
             << "workDir=" << m_serverConfig.workDir;
}

void MainWindow::setupToolBar(){
    toolBar = addToolBar("Управление сервером");
    toolBar->setMovable(false);
    toolBar->setIconSize(QSize(24,24));

    // Управление сервером
    QStyle *st = QApplication::style();
    actionStart = toolBar->addAction(st->standardIcon(QStyle::SP_MediaPlay),  "Запуск");
    actionStop  = toolBar->addAction(st->standardIcon(QStyle::SP_MediaStop),  "Стоп");
    actionReset = toolBar->addAction(st->standardIcon(QStyle::SP_BrowserReload), "Перезапуск");

    toolBar->addSeparator();

    // CRUD зон
    actionAddZone    = toolBar->addAction("Создать зону");
    actionEditZone   = toolBar->addAction("Свойства зоны");
    actionDeleteZone = toolBar->addAction("Удалить зону");

    toolBar->addSeparator();

    // CRUD записей
    actionAddRecord    = toolBar->addAction("Добавить запись");
    actionEditRecord   = toolBar->addAction("Изменить запись");
    actionDeleteRecord = toolBar->addAction("Удалить запись");

    toolBar->addSeparator();
    QAction *actionSettings = toolBar->addAction(
        st->standardIcon(QStyle::SP_FileDialogDetailedView), "Настройки");
    toolBar->addSeparator();
    toolBar->addAction("Помощь");

    connect(actionStart,  &QAction::triggered, this, &MainWindow::onStartServer);
    connect(actionStop,   &QAction::triggered, this, &MainWindow::onStopServer);
    connect(actionReset,  &QAction::triggered, this, &MainWindow::onRestartServer);

    connect(actionAddZone,    &QAction::triggered, this, &MainWindow::onAddZone);
    connect(actionEditZone,   &QAction::triggered, this, &MainWindow::onEditZone);
    connect(actionDeleteZone, &QAction::triggered, this, &MainWindow::onDeleteZone);

    connect(actionAddRecord,    &QAction::triggered, this, &MainWindow::onAddRecord);
    connect(actionEditRecord,   &QAction::triggered, this, &MainWindow::onEditRecord);
    connect(actionDeleteRecord, &QAction::triggered, this, &MainWindow::onDeleteRecord);

    connect(actionSettings, &QAction::triggered, this, &MainWindow::onSettings);
}


//Центральный виджит
void MainWindow::setupCentralWidget(){
    splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);

    setupTreePanel();
    setupTablePanel();

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);

    setCentralWidget(splitter);

}

void MainWindow::setupTreePanel(){
    treeWidget = new QTreeWidget(splitter);
    treeWidget->setHeaderLabel("DNS");
    treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    treeWidget->setAnimated(true);

    // Контекстное меню по ПКМ
    connect(treeWidget, &QTreeWidget::customContextMenuRequested,
            this, &MainWindow::onTreeContextMenu);

    // Реакция на выбор элемента
    connect(treeWidget, &QTreeWidget::itemClicked,
            this, &MainWindow::onTreeItemSelected);

    // Двойной клик по зоне — редактировать
    connect(treeWidget, &QTreeWidget::itemDoubleClicked,
            this, [this](QTreeWidgetItem *item, int) {
                if (currentZoneItem() == item)
                    onEditZone();
            });

    splitter->addWidget(treeWidget);

}

void MainWindow::setupTablePanel(){
    tableWidget = new QTableWidget(splitter);

    tableWidget->setColumnCount(4);
    tableWidget->setHorizontalHeaderLabels({"Имя","Тип", "TTL", "Данные"});

    // Настройки отображения
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableWidget->setAlternatingRowColors(true);

    // Двойной клик по строке — редактировать запись
    connect(tableWidget, &QTableWidget::itemDoubleClicked,
            this, [this](QTableWidgetItem *) { onEditRecord(); });

    // Смена выделения в таблице — обновить состояние кнопок
    connect(tableWidget, &QTableWidget::currentCellChanged,
            this, [this](int, int, int, int) { updateActions(); });

    splitter->addWidget(tableWidget);
}

//Статус бар
void MainWindow::setupStatusBar() {
    labelBindVersion  = new QLabel("BIND: не определён");
    labelServerStatus = new QLabel();
    labelServerStatus->setTextFormat(Qt::RichText);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 0);
    m_progressBar->setFixedWidth(120);
    m_progressBar->setVisible(false);

    statusBar()->addWidget(labelBindVersion);
    statusBar()->addWidget(m_progressBar);
    statusBar()->addPermanentWidget(labelServerStatus);

    m_statusTimer = new QTimer(this);
    m_statusTimer->setInterval(5000);
    connect(m_statusTimer, &QTimer::timeout, this, &MainWindow::refreshStatus);
    m_statusTimer->start();

    refreshStatus();
}

void MainWindow::refreshStatus() {
    bool running = m_bindManager->isRunning();
    QString ver  = m_bindManager->version();

    qDebug() << "[MainWindow] refreshStatus: running=" << running << "version=" << ver;

    if (running) {
        labelServerStatus->setText("<font color='#2ecc71'>●</font> Активен");
        actionStart->setEnabled(false);
        actionStop->setEnabled(true);
        actionReset->setEnabled(true);
    } else {
        labelServerStatus->setText("<font color='#e74c3c'>●</font> Остановлен");
        actionStart->setEnabled(true);
        actionStop->setEnabled(false);
        actionReset->setEnabled(false);
    }
    labelBindVersion->setText(ver.isEmpty() ? "BIND: не определён" : "BIND: " + ver);
}

void MainWindow::setServerBusy(bool busy, const QString &message) {
    qDebug() << "[MainWindow] setServerBusy: busy=" << busy << "message=" << message;
    m_progressBar->setVisible(busy);
    if (busy) {
        labelServerStatus->setText("<font color='#f39c12'>●</font> " + message);
        statusBar()->showMessage(message);
        actionStart->setEnabled(false);
        actionStop->setEnabled(false);
        actionReset->setEnabled(false);
    } else {
        statusBar()->clearMessage();
        refreshStatus();
    }
}

void MainWindow::onSettings() {
    qDebug() << "[MainWindow] onSettings: открытие диалога настроек";
    SettingsDialog dlg(m_serverConfig, this);
    if (dlg.exec() != QDialog::Accepted) return;

    m_serverConfig = dlg.config();
    saveSettings();
    qDebug() << "[MainWindow] onSettings: applied new config, namedConfPath="
             << m_serverConfig.namedConfPath;

    // Перезагрузить дерево зон с новыми настройками
    treeWidget->clear();
    itemServer = nullptr;
    populateTree();
}

void MainWindow::onServerCommandFinished(const QString &action, bool success, const QString &error) {
    qDebug() << "[MainWindow] onServerCommandFinished: action=" << action
             << "success=" << success;
    setServerBusy(false);
    if (!success) {
        QMessageBox::critical(this, "Ошибка операции с сервером",
                               "Команда «" + action + "» завершилась с ошибкой:\n" + error);
    } else {
        statusBar()->showMessage("Команда «" + action + "» выполнена успешно", 3000);
    }
    refreshStatus();
}

//Заполнение начальными значениями
void MainWindow::populateTree() {
    // Корневой узел — сервер
    itemServer = new QTreeWidgetItem(treeWidget);
    itemServer->setText(0, "localhost");


    // Дочерние узлы
    itemForwardZones = new QTreeWidgetItem(itemServer);
    itemForwardZones->setText(0, "Зоны прямого просмотра");

    itemReverseZones = new QTreeWidgetItem(itemServer);
    itemReverseZones->setText(0, "Зоны обратного просмотра");

    itemEventLog = new QTreeWidgetItem(itemServer);
    itemEventLog->setText(0, "Просмотр событий");

    // Раскрываем корень
    treeWidget->expandAll();

    QString error;
    QList<Zone> zones = m_bindManager->loadZones(m_serverConfig.namedConfPath, &error);
    if (!error.isEmpty()) {
        statusBar()->showMessage("Ошибка загрузки: " + error);
        return;
    }

    for (const Zone &zone : zones) {
        QTreeWidgetItem *zoneItem = new QTreeWidgetItem();
        zoneItem->setText(0, zone.name);

            // Кладём путь к файлу зоны в UserRole —
            // потом достанем когда пользователь кликнет на зону
        zoneItem->setData(0, Qt::UserRole, zone.filePath);

            // Определяем куда добавить - прямая или обратная зона
         if (zone.name.contains("in-addr.arpa") ||
              zone.name.contains("ip6.arpa")) {
              itemReverseZones->addChild(zoneItem);
          } else {
              itemForwardZones->addChild(zoneItem);
          }
      }

     treeWidget->expandAll();

}


//Слоты управления сервером
void MainWindow::onStartServer() {
    qDebug() << "[MainWindow] onStartServer";
    setServerBusy(true, "Запускаю сервер...");
    m_bindManager->startAsync();
}

void MainWindow::onStopServer() {
    qDebug() << "[MainWindow] onStopServer";
    setServerBusy(true, "Останавливаю сервер...");
    m_bindManager->stopAsync();
}

void MainWindow::onRestartServer() {
    qDebug() << "[MainWindow] onRestartServer";
    setServerBusy(true, "Перезапускаю сервер...");
    m_bindManager->restartAsync();
}


//выбор элемента дерева и обновить таблицу
void MainWindow::onTreeItemSelected(QTreeWidgetItem *item, int /*column*/) {
    tableWidget->setRowCount(0); // очищаем таблицу

    // Определяем что выбрано и заполняем таблицу нужными данными
    if (item == itemEventLog) {
        // Потом здесь: загрузить и отобразить журнал
        tableWidget->setHorizontalHeaderLabels({"Время", "Уровень", "Сообщение", ""});
        return;
    }

    // Если выбрана зона — показываем записи
    if (item->parent() == itemForwardZones ||
        item->parent() == itemReverseZones) {

        tableWidget->setHorizontalHeaderLabels({"Имя", "Тип", "TTL", "Данные"});

        QString filePath = item->data(0, Qt::UserRole).toString();

        if (filePath.isEmpty()) {
            int row = tableWidget->rowCount();
            tableWidget->insertRow(row);
            tableWidget->setItem(row, 0, new QTableWidgetItem("(путь к файлу зоны не задан)"));
            return;
        }

        QString error;
        const QList<ResourceRecord> records = m_bindManager->loadZoneRecords(filePath, &error);
        if (!error.isEmpty()) {
            qWarning() << "[MainWindow] loadZoneRecords error:" << error;
            int row = tableWidget->rowCount();
            tableWidget->insertRow(row);
            tableWidget->setItem(row, 0, new QTableWidgetItem("Ошибка: " + error));
            return;
        }

        for (const ResourceRecord &rr : records) {
            QString data = rr.data;
            if (rr.type == RecordType::MX && rr.priority > 0)
                data = QString::number(rr.priority) + " " + rr.data;

            int row = tableWidget->rowCount();
            tableWidget->insertRow(row);
            tableWidget->setItem(row, 0, new QTableWidgetItem(rr.name));
            tableWidget->setItem(row, 1, new QTableWidgetItem(recordTypeToString(rr.type)));
            tableWidget->setItem(row, 2, new QTableWidgetItem(rr.ttl ? QString::number(rr.ttl) : ""));
            tableWidget->setItem(row, 3, new QTableWidgetItem(data));
        }
    }

    updateActions();
}

void MainWindow::updateActions() {
    const bool hasZone = (currentZoneItem() != nullptr);
    const bool hasRow  = (tableWidget->currentRow() >= 0);

    actionAddZone->setEnabled(true);
    actionEditZone->setEnabled(hasZone);
    actionDeleteZone->setEnabled(hasZone);

    actionAddRecord->setEnabled(hasZone);
    actionEditRecord->setEnabled(hasZone && hasRow);
    actionDeleteRecord->setEnabled(hasZone && hasRow);

    qDebug() << "[MainWindow] updateActions: hasZone=" << hasZone << "hasRow=" << hasRow;
}

QTreeWidgetItem *MainWindow::currentZoneItem() const {
    QTreeWidgetItem *item = treeWidget->currentItem();
    if (!item) return nullptr;
    if (item->parent() == itemForwardZones || item->parent() == itemReverseZones)
        return item;
    return nullptr;
}

void MainWindow::onAddZone() {
    qDebug() << "[MainWindow] onAddZone";
    ZoneDialog dlg(Zone{}, m_serverConfig.workDir, this);
    if (dlg.exec() != QDialog::Accepted) return;

    Zone zone = dlg.zone();
    if (zone.name.isEmpty()) return;

    QString error;
    if (!m_bindManager->saveZone(zone, m_serverConfig.namedConfPath, &error)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось сохранить зону:\n" + error);
        return;
    }

    QTreeWidgetItem *zoneItem = new QTreeWidgetItem();
    zoneItem->setText(0, zone.name);
    zoneItem->setData(0, Qt::UserRole, zone.filePath);

    if (zone.view == ZoneView::Reverse)
        itemReverseZones->addChild(zoneItem);
    else
        itemForwardZones->addChild(zoneItem);

    treeWidget->setCurrentItem(zoneItem);
    statusBar()->showMessage("Зона " + zone.name + " создана");
}

void MainWindow::onEditZone() {
    QTreeWidgetItem *item = currentZoneItem();
    if (!item) return;

    const QString zoneName = item->text(0);
    const QString filePath = item->data(0, Qt::UserRole).toString();
    qDebug() << "[MainWindow] onEditZone, zone=" << zoneName;

    Zone zone;
    zone.name     = zoneName;
    zone.filePath = filePath;
    zone.view     = (item->parent() == itemReverseZones) ? ZoneView::Reverse : ZoneView::Forward;

    ZoneDialog dlg(zone, m_serverConfig.workDir, this);
    if (dlg.exec() != QDialog::Accepted) return;

    Zone updated = dlg.zone();
    qDebug() << "[MainWindow] onEditZone: обновлённая зона=" << updated.name << "filePath=" << updated.filePath;

    QString error;
    if (!m_bindManager->saveZone(updated, m_serverConfig.namedConfPath, &error)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось сохранить зону:\n" + error);
        return;
    }

    item->setText(0, updated.name);
    item->setData(0, Qt::UserRole, updated.filePath);
    statusBar()->showMessage("Зона " + updated.name + " обновлена");
}

void MainWindow::onDeleteZone() {
    qDebug() << "[MainWindow] onDeleteZone";
    QTreeWidgetItem *item = currentZoneItem();
    if (!item) return;

    const QString zoneName = item->text(0);
    const QString filePath = item->data(0, Qt::UserRole).toString();

    int ret = QMessageBox::question(this, "Удалить зону",
                                     "Удалить зону «" + zoneName + "»?\nФайл " + filePath + " будет удалён.",
                                     QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    QString error;
    if (!m_bindManager->deleteZone(zoneName, filePath, m_serverConfig.namedConfPath, &error)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось удалить зону:\n" + error);
        return;
    }

    QTreeWidgetItem *parent = item->parent();
    parent->removeChild(item);
    tableWidget->setRowCount(0);
    statusBar()->showMessage("Зона " + zoneName + " удалена");
}

void MainWindow::onAddRecord() {
    qDebug() << "[MainWindow] onAddRecord";
    QTreeWidgetItem *zoneItem = currentZoneItem();
    if (!zoneItem) return;

    const QString zoneName = zoneItem->text(0);
    const QString filePath = zoneItem->data(0, Qt::UserRole).toString();

    RecordDialog dlg(ResourceRecord{}, this);
    if (dlg.exec() != QDialog::Accepted) return;

    ResourceRecord newRr = dlg.record();

    // Загрузить текущие записи, добавить новую, сохранить зону
    QString error;
    QList<ResourceRecord> records = m_bindManager->loadZoneRecords(filePath, &error);
    records.append(newRr);

    Zone zone;
    zone.name     = zoneName;
    zone.filePath = filePath;
    zone.records  = records;
    zone.view     = (zoneItem->parent() == itemReverseZones) ? ZoneView::Reverse : ZoneView::Forward;

    if (!m_bindManager->saveZone(zone, m_serverConfig.namedConfPath, &error)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось сохранить запись:\n" + error);
        return;
    }

    // Добавить строку в таблицу
    QString data = newRr.data;
    if (newRr.type == RecordType::MX && newRr.priority > 0)
        data = QString::number(newRr.priority) + " " + newRr.data;

    int row = tableWidget->rowCount();
    tableWidget->insertRow(row);
    tableWidget->setItem(row, 0, new QTableWidgetItem(newRr.name));
    tableWidget->setItem(row, 1, new QTableWidgetItem(recordTypeToString(newRr.type)));
    tableWidget->setItem(row, 2, new QTableWidgetItem(newRr.ttl ? QString::number(newRr.ttl) : ""));
    tableWidget->setItem(row, 3, new QTableWidgetItem(data));

    statusBar()->showMessage("Запись добавлена");
}

void MainWindow::onEditRecord() {
    QTreeWidgetItem *zoneItem = currentZoneItem();
    if (!zoneItem) return;

    int row = tableWidget->currentRow();
    if (row < 0) return;

    const QString zoneName = zoneItem->text(0);
    const QString filePath = zoneItem->data(0, Qt::UserRole).toString();
    qDebug() << "[MainWindow] onEditRecord row=" << row << "zone=" << zoneName;

    QString error;
    QList<ResourceRecord> records = m_bindManager->loadZoneRecords(filePath, &error);
    if (!error.isEmpty()) {
        QMessageBox::critical(this, "Ошибка", "Не удалось загрузить записи:\n" + error);
        return;
    }
    if (row >= records.size()) {
        qWarning() << "[MainWindow] onEditRecord: row" << row << ">= records.size()" << records.size();
        return;
    }

    RecordDialog dlg(records[row], this);
    if (dlg.exec() != QDialog::Accepted) return;

    records[row] = dlg.record();
    const ResourceRecord &rr = records[row];
    qDebug() << "[MainWindow] onEditRecord: обновлённая запись name=" << rr.name << "type=" << static_cast<int>(rr.type);

    Zone zone;
    zone.name     = zoneName;
    zone.filePath = filePath;
    zone.records  = records;
    zone.view     = (zoneItem->parent() == itemReverseZones) ? ZoneView::Reverse : ZoneView::Forward;

    if (!m_bindManager->saveZone(zone, m_serverConfig.namedConfPath, &error)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось сохранить запись:\n" + error);
        return;
    }

    // Обновить строку таблицы
    QString data = rr.data;
    if (rr.type == RecordType::MX && rr.priority > 0)
        data = QString::number(rr.priority) + " " + rr.data;

    tableWidget->setItem(row, 0, new QTableWidgetItem(rr.name));
    tableWidget->setItem(row, 1, new QTableWidgetItem(recordTypeToString(rr.type)));
    tableWidget->setItem(row, 2, new QTableWidgetItem(rr.ttl ? QString::number(rr.ttl) : ""));
    tableWidget->setItem(row, 3, new QTableWidgetItem(data));

    statusBar()->showMessage("Запись обновлена");
    qDebug() << "[MainWindow] onEditRecord: запись row=" << row << "обновлена";
}

void MainWindow::onDeleteRecord() {
    QTreeWidgetItem *zoneItem = currentZoneItem();
    if (!zoneItem) return;

    int row = tableWidget->currentRow();
    if (row < 0) return;

    const QString zoneName = zoneItem->text(0);
    const QString filePath = zoneItem->data(0, Qt::UserRole).toString();
    qDebug() << "[MainWindow] onDeleteRecord row=" << row << "zone=" << zoneName;

    QString error;
    QList<ResourceRecord> records = m_bindManager->loadZoneRecords(filePath, &error);
    if (!error.isEmpty()) {
        QMessageBox::critical(this, "Ошибка", "Не удалось загрузить записи:\n" + error);
        return;
    }
    if (row >= records.size()) {
        qWarning() << "[MainWindow] onDeleteRecord: row" << row << ">= records.size()" << records.size();
        return;
    }

    records.removeAt(row);

    Zone zone;
    zone.name     = zoneName;
    zone.filePath = filePath;
    zone.records  = records;
    zone.view     = (zoneItem->parent() == itemReverseZones) ? ZoneView::Reverse : ZoneView::Forward;

    if (!m_bindManager->saveZone(zone, m_serverConfig.namedConfPath, &error)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось сохранить изменения:\n" + error);
        return;
    }

    tableWidget->removeRow(row);
    statusBar()->showMessage("Запись удалена");
    qDebug() << "[MainWindow] onDeleteRecord: запись удалена, строк осталось" << tableWidget->rowCount();
}

void MainWindow::onTreeContextMenu(const QPoint &pos) {
    QTreeWidgetItem *item = treeWidget->itemAt(pos);
    if (!item) return;

    treeWidget->setCurrentItem(item);
    QMenu menu(this);

    if (item == itemServer) {
        menu.addAction("Запустить сервер",   this, &MainWindow::onStartServer);
        menu.addAction("Остановить сервер",  this, &MainWindow::onStopServer);
        menu.addAction("Перезапустить",      this, &MainWindow::onRestartServer);

    } else if (item == itemForwardZones || item == itemReverseZones) {
        menu.addAction("Создать зону...", this, &MainWindow::onAddZone);

    } else if (item->parent() == itemForwardZones ||
               item->parent() == itemReverseZones) {
        menu.addAction("Добавить запись...",  this, &MainWindow::onAddRecord);
        menu.addAction("Свойства записи...",  this, &MainWindow::onEditRecord);
        menu.addAction("Удалить запись",      this, &MainWindow::onDeleteRecord);
        menu.addSeparator();
        menu.addAction("Свойства зоны...",    this, &MainWindow::onEditZone);
        menu.addAction("Удалить зону",        this, &MainWindow::onDeleteZone);

    } else if (item == itemEventLog) {
        menu.addAction("Обновить журнал");
        menu.addAction("Очистить отображение");
    }

    menu.exec(treeWidget->viewport()->mapToGlobal(pos));
}

MainWindow::~MainWindow()
{
}

