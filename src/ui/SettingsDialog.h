#pragma once
#include <QDialog>
#include "model/ServerConfig.h"

class QLineEdit;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(const ServerConfig &cfg, QWidget *parent = nullptr);

    ServerConfig config() const;

private slots:
    void onBrowseNamedConf();
    void onBrowseWorkDir();

private:
    QLineEdit *m_namedConfPath;
    QLineEdit *m_workDir;
};
