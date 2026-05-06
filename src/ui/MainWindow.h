#pragma once
#include <QMainWindow>
#include <QToolBar>
#include <QAction>
#include <QSplitter>
#include <QTimer>
#include <QTreeWidget>
#include <QTableWidget>
#include <QLabel>
#include "dns/BindManager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Управление сервером
    void onStartServer();
    void onStopServer();
    void onRestartServer();

    // Выбор узла в дереве
    void onTreeItemSelected(QTreeWidgetItem *item, int column);
    void onTreeContextMenu(const QPoint &pos);

    // CRUD зон
    void onAddZone();
    void onDeleteZone();

    // CRUD записей
    void onAddRecord();
    void onDeleteRecord();

    // Статусбар
    void refreshStatus();

private:
    // Методы UI
    void setupToolBar();
    void setupCentralWidget();
    void setupTreePanel();
    void setupTablePanel();
    void setupStatusBar();
    void populateTree();

    // Вспомогательные методы
    QTreeWidgetItem *currentZoneItem() const;
    QString          namedConfPath()   const { return m_namedConfPath; }

    // Виджеты
    QToolBar    *toolBar;
    QSplitter   *splitter;
    QTreeWidget *treeWidget;
    QTableWidget *tableWidget;

    QTreeWidgetItem *itemServer;
    QTreeWidgetItem *itemForwardZones;
    QTreeWidgetItem *itemReverseZones;
    QTreeWidgetItem *itemEventLog;

    // Статус
    QLabel  *labelBindVersion;
    QLabel  *labelServerStatus;
    QTimer  *m_statusTimer;

    // Actions
    QAction *actionStart;
    QAction *actionStop;
    QAction *actionReset;

    BindManager m_bindManager;
    QString     m_namedConfPath = "/etc/bind/named.conf";
};
