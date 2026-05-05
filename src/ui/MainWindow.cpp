#include "ui/MainWindow.h"
#include "ui_mainwindow.h"
#include "parser/NamedConfParser.h"
#include "parser/ZoneFileParser.h"

#include <QDebug>
#include <QMessageBox>

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



MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("dns-manager-admin");
    setMinimumSize(900, 600);
    resize(1100, 700);

    setupToolBar();
    setupCentralWidget();
    setupStatusBar();
    populateTree();
}

void MainWindow::setupToolBar(){
    toolBar = addToolBar("Управление сервером");
    toolBar->setMovable(false);
    toolBar->setIconSize(QSize(24,24));

    //Кнопки
    actionStart = toolBar->addAction("Запуск");
    actionStop = toolBar->addAction("Стоп");
    actionReset = toolBar->addAction("Перезапуск");

    toolBar->addSeparator();
    toolBar->addAction("Настройки");
    toolBar->addSeparator();
    toolBar->addAction("Помощь");

    connect(actionStart, &QAction::triggered, this, &MainWindow::onStartServer);
    connect(actionStop, &QAction::triggered, this, &MainWindow::onStopServer);
    connect(actionReset, &QAction::triggered, this, &MainWindow::onRestartServer);
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


    splitter->addWidget(tableWidget);
}

//Статус бар
void MainWindow::setupStatusBar() {
    labelBindVersion  = new QLabel("BIND: не определён");
    labelServerStatus = new QLabel("● Статус: неизвестно");

    statusBar()->addWidget(labelBindVersion);
    statusBar()->addPermanentWidget(labelServerStatus);
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

    NamedConfParser parser;

    QList<Zone> zones = parser.parse("/etc/bind/named.conf");

    if (!parser.lastError().isEmpty()) {
           statusBar()->showMessage("Ошибка загрузки: " + parser.lastError());
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


//Слоты
void MainWindow::onStartServer(){


    labelServerStatus->setText("● Статус: активен");

    QMessageBox::information(this, "Запуск", "Сервер запущен(Заглушка)");
}

void MainWindow::onStopServer(){

    labelServerStatus->setText("○ Статус: остановлен");

    QMessageBox::information(this, "Стоп", "Сервер остановлен");
}

void MainWindow::onRestartServer(){

    labelServerStatus->setText("● Статус: активен");

    QMessageBox::information(this, "Перезапуск", "Сервер перезапущен");
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

        qDebug() << "ZoneFileParser: загружаем" << filePath;
        ZoneFileParser parser;
        const QList<ResourceRecord> records = parser.parse(filePath);

        if (!parser.lastError().isEmpty()) {
            qWarning() << "ZoneFileParser ошибка:" << parser.lastError();
            int row = tableWidget->rowCount();
            tableWidget->insertRow(row);
            tableWidget->setItem(row, 0, new QTableWidgetItem("Ошибка: " + parser.lastError()));
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
}

void MainWindow::onTreeContextMenu(const QPoint &pos) {
    QTreeWidgetItem *item = treeWidget->itemAt(pos);
    if (!item) return;

    QMenu menu(this);

    if (item == itemServer) {
        menu.addAction("Запустить сервер",   this, &MainWindow::onStartServer);
        menu.addAction("Остановить сервер",  this, &MainWindow::onStopServer);
        menu.addAction("Перезапустить",      this, &MainWindow::onRestartServer);
        menu.addSeparator();
        menu.addAction("Проверить конфигурацию");

    } else if (item == itemForwardZones || item == itemReverseZones) {
        menu.addAction("Создать зону...");

    } else if (item->parent() == itemForwardZones ||
               item->parent() == itemReverseZones) {
        menu.addAction("Создать запись...");
        menu.addAction("Удалить зону");
        menu.addSeparator();
        menu.addAction("Свойства зоны...");

    } else if (item == itemEventLog) {
        menu.addAction("Обновить журнал");
        menu.addAction("Очистить отображение");
    }

    menu.exec(treeWidget->viewport()->mapToGlobal(pos));
}

MainWindow::~MainWindow()
{
}

