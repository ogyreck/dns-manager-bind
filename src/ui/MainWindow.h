#pragma once
#include <QMainWindow>
#include <QToolBar>
#include <QAction>
#include <QSplitter>
#include <QTimer>
#include <QTreeWidget>
#include <QTableWidget>
#include <QLabel>
#include <QProgressBar>
#include "dns/BindManager.h"
#include "model/EventLogEntry.h"
#include "model/ServerConfig.h"

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
    void onEditZone();
    void onDeleteZone();

    // CRUD записей
    void onAddRecord();
    void onEditRecord();
    void onDeleteRecord();

    // Статусбар
    void refreshStatus();

    // Управление активностью кнопок
    void updateActions();

    // Настройки
    void onSettings();

    // Операции с сервером (async)
    void onServerCommandFinished(const QString &action, bool success, const QString &error);

    // Журнал событий
    void refreshEventLog();

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
    void             loadSettings();
    void             saveSettings();
    void             setServerBusy(bool busy, const QString &message = {});

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
    QLabel       *labelBindVersion;
    QLabel       *labelServerStatus;
    QProgressBar *m_progressBar;
    QTimer       *m_statusTimer;

    // Actions — управление сервером
    QAction *actionStart;
    QAction *actionStop;
    QAction *actionReset;

    // Actions — CRUD зон и записей
    QAction *actionAddZone;
    QAction *actionEditZone;
    QAction *actionDeleteZone;
    QAction *actionAddRecord;
    QAction *actionEditRecord;
    QAction *actionDeleteRecord;

    BindManager  *m_bindManager;
    ServerConfig  m_serverConfig;
};
