#pragma once
#include <QDialog>
#include "model/Zone.h"

class QLineEdit;
class QComboBox;
class QPushButton;

class ZoneDialog : public QDialog {
    Q_OBJECT
public:
    explicit ZoneDialog(const Zone &zone = Zone{},
                        const QString &workDir = "/etc/bind",
                        QWidget *parent = nullptr);

    Zone zone() const;

public slots:
    void accept() override;

private slots:
    void onTypeChanged(int index);
    void onBrowse();
    void onNameChanged(const QString &name);
    void onFilePathEdited();

private:
    QLineEdit   *m_name;
    QComboBox   *m_type;
    QLineEdit   *m_filePath;
    QPushButton *m_browse;
    QLineEdit   *m_masterIp;
    QString      m_workDir;
    bool         m_filePathEdited = false;
};
