#pragma once
#include <QDialog>
#include "model/ResourceRecord.h"

class QLineEdit;
class QComboBox;
class QSpinBox;

class RecordDialog : public QDialog {
    Q_OBJECT
public:
    explicit RecordDialog(const ResourceRecord &rr = ResourceRecord{}, QWidget *parent = nullptr);

    ResourceRecord record() const;

public slots:
    void accept() override;

private slots:
    void onTypeChanged(int index);

private:
    QLineEdit *m_name;
    QComboBox *m_type;
    QSpinBox  *m_ttl;
    QLineEdit *m_data;
    QSpinBox  *m_priority;
    QWidget   *m_priorityRow; // для показа/скрытия строки Priority
};
