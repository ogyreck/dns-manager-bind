#include "ui/HelpDialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>

HelpDialog::HelpDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("О программе");
    setMinimumWidth(360);
    setModal(true);

    auto *layout = new QVBoxLayout(this);

    auto *lblTitle = new QLabel("<b>dns-manager-admin</b>", this);
    lblTitle->setAlignment(Qt::AlignCenter);

    auto *lblVersion = new QLabel(QString("Версия: %1").arg(APP_VERSION), this);
    lblVersion->setAlignment(Qt::AlignCenter);

    auto *lblInfo = new QLabel(
        "Учебный проект.\n\n"
        "Автор: Передвигин Андрей\n"
        "Студент группы 4242",
        this);
    lblInfo->setAlignment(Qt::AlignCenter);
    lblInfo->setWordWrap(true);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);

    layout->addWidget(lblTitle);
    layout->addWidget(lblVersion);
    layout->addSpacing(8);
    layout->addWidget(lblInfo);
    layout->addSpacing(8);
    layout->addWidget(buttons);
}
