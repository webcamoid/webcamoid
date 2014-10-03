/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#include "ui_configdialog.h"

#include "configdialog.h"

ConfigDialog::ConfigDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConfigDialog)
{
    this->ui->setupUi(this);

    QPushButton *btnOk = this->ui->bbxControls->button(QDialogButtonBox::Ok);
    QObject::connect(btnOk, SIGNAL(clicked()), this, SIGNAL(okClicked()));
    QObject::connect(btnOk, SIGNAL(clicked()), this, SLOT(close()));

    QPushButton *btnCancel = this->ui->bbxControls->button(QDialogButtonBox::Cancel);
    QObject::connect(btnCancel, SIGNAL(clicked()), this, SIGNAL(cancelClicked()));
    QObject::connect(btnCancel, SIGNAL(clicked()), this, SLOT(close()));
}

void ConfigDialog::addPage(QWidget *page, const QString &itemName, const QIcon &icon, const QString &header)
{
    QListWidgetItem *item = new QListWidgetItem(icon, itemName, this->ui->lstOptions);
    Q_UNUSED(item)

    QWidget *widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout();
    widget->setLayout(layout);
    layout->addWidget(new QLabel(QString("<b>%1</b>").arg(header)));
    layout->addWidget(page);

    this->ui->stackedWidget->addWidget(widget);
}

void ConfigDialog::on_lstOptions_itemClicked(QListWidgetItem *item)
{
    int page = this->ui->lstOptions->row(item);
    this->ui->stackedWidget->setCurrentIndex(page);
}
