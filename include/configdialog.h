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
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QListWidgetItem>

#include "commons.h"
#include "appenvironment.h"

namespace Ui
{
    class ConfigDialog;
}

class COMMONSSHARED_EXPORT ConfigDialog: public QDialog
{
    Q_OBJECT

    public:
        explicit ConfigDialog(QWidget *parent = 0);
        ~ConfigDialog();

    private:
        QSharedPointer<Ui::ConfigDialog> ui;

        AppEnvironment *m_appEnvironment;

    signals:
        void okClicked();
        void cancelClicked();

    public slots:
        void addPage(QWidget *page,
                     const QString &itemName,
                     const QIcon &icon = QIcon(),
                     const QString &header= QString());

    private slots:
        void on_lstOptions_itemClicked(QListWidgetItem *item);
};

#endif // CONFIGDIALOG_H
